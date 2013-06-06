//
//	OpenEXR file importer/exporter for After Effects (AEIO)
// 
//	by Brendan Bolles <brendan@fnordware.com>
//
//	History
//	1.0		originally started in May 2007, beta in June, released August 20
//	1.1		17 Sep 2007 - performance enhancements, etc
//	1.2		2 Nov 2007 - Writing more metadata like project and comp name
//	1.3		12 Sep 2008 - Bug fixes
//	1.?		?? - Bug fixes, Tuned Multithreading code, ...?
//	1.5		15 November 2009 - Updated for AE10, real ICC Profile support
//	1.6		27 October 2011 - Performance enhancements, using iterate suites where possible
//	1.7		26 April 2012 - More performance enhancements, unlimited number of channels
//	1.8		1 January 2013 - AE channel cache, UTF-8 support
//
//
//	ToDo in this module:
//
//	Option to save with other resolutions included, maybe read them when advantageous
//	Save as tiles instead of always scanline
//	Create/use preview images
//	Handle other metadata I might be missing (?)
//
//
//	ToDo in other plug-ins:
//
//	Add layer pulldown to EXtractoR to automatically set RGB channels
//	Add other Shift Channels-like options to EXtractoR (maybe....for Ben Grossmann)
//	Some way to automatically set ranges for things like Z-Depth (maybe sample the CW with mouse?)
//	Do something with Coverage in IDentifier (?)
//	AEGP to turn an environment map into a 3D comp in AE
//	AEGP to get camera move from frame sequence (like AE can do with RLA)
//

#include "OpenEXR.h"
#include "OpenEXR_PlatformIO.h"
#include "OpenEXR_UI.h"
#include "OpenEXR_ChannelMap.h"
#include "OpenEXR_ChannelCache.h"
#include "OpenEXR_iccProfileAttribute.h"


#include "ImfHybridInputFile.h"
#include <ImfOutputFile.h>
#include <ImfRgbaFile.h>

#include <ImfChannelList.h>
#include <ImfVersion.h>

#include <ImfStandardAttributes.h>

#include <ImfArray.h>

#include <IexBaseExc.h>

#include <list>

#ifndef __MACH__
#include <assert.h>
#include <time.h>
#include <sys/timeb.h>
#endif


#ifdef MAC_ENV
	#include <mach/mach.h>
#endif

#include <IlmThread.h>
#include <IlmThreadPool.h>


using namespace std;
using namespace Imf;
using namespace Imath;
using namespace Iex;


extern AEGP_PluginID S_mem_id;

// global Channel Map, set up when plug-in loads
static ChannelMap *gChannelMap = NULL;

// our prefs
static A_long gChannelCaches = 3;
static A_long gCacheTimeout = 30;
static A_long gAutoCacheChannels = 0;
static A_Boolean gMemoryMap = FALSE;
static A_Boolean gStorePersonal = FALSE;
static A_Boolean gStoreMachine = FALSE;


static OpenEXR_CachePool gCachePool;


A_Err
OpenEXR_Init(struct SPBasicSuite *pica_basicP)
{
	A_Err err = A_Err_NONE;

	if( IlmThread::supportsThreads() )
	{
#ifdef MAC_ENV
		// get number of CPUs using Mach calls
		host_basic_info_data_t hostInfo;
		mach_msg_type_number_t infoCount;
		
		infoCount = HOST_BASIC_INFO_COUNT;
		host_info(mach_host_self(), HOST_BASIC_INFO, 
				  (host_info_t)&hostInfo, &infoCount);
		
		setGlobalThreadCount(hostInfo.max_cpus);
#else // WIN_ENV
		SYSTEM_INFO systemInfo;
		GetSystemInfo(&systemInfo);

		setGlobalThreadCount(systemInfo.dwNumberOfProcessors);
#endif
	}

	staticInitialize();
	iccProfileAttribute::registerAttributeType();
	
#ifdef WIN_ENV
	#define PATH_DELIMITER	"\\"
#else
	#define PATH_DELIMITER	"/"
#endif

	// get plug-in path so we can find the channel mapping file
	A_char	plugin_pathZ[AEGP_MAX_PATH_SIZE] = {'\0'};
	OpenEXR_CopyPluginPath(plugin_pathZ, AEGP_MAX_PATH_SIZE-1);


	// set up channel mapping
	string table_path(plugin_pathZ);
	table_path.resize( table_path.find_last_of( PATH_DELIMITER ) + 1 );
	table_path += "OpenEXR_channel_map.txt"; // the file we're looking for;
	
	gChannelMap = new ChannelMap( table_path.c_str() );
	
	if(gChannelMap && !gChannelMap->exists())
	{
		// no file, let's add some basic defaults
		gChannelMap->addEntry( ChannelEntry("Z", PF_ChannelType_DEPTH, PF_DataType_FLOAT) );
	}
	
	
	// get our prefs
#define PREFS_SECTION	"OpenEXR"
#define PREFS_CHANNEL_CACHES "Channel Caches Number"
#define PREFS_CACHE_EXPIRATION "Channel Cache Expiration"
#define PREFS_AUTO_CACHE "Auto Cache Threshold"
#define PREFS_MEMORY_MAP	"Memory Map"
#define PREFS_PERSONAL_INFO "Store Personal Info"
#define PREFS_MACHINE_INFO	"Store Machine Info"
	
	AEGP_SuiteHandler suites(pica_basicP);
	
	AEGP_PersistentBlobH blobH = NULL;
	suites.PersistentDataSuite()->AEGP_GetApplicationBlob(&blobH);
	
	A_long channel_caches = gChannelCaches; // defaults come from global initializations above
	A_long cache_timeout = gCacheTimeout;
	A_long auto_cache_channels = gAutoCacheChannels;
	A_long memory_map = gMemoryMap;
	A_long store_personal = gStorePersonal;
	A_long store_machine = gStoreMachine;
	
	suites.PersistentDataSuite()->AEGP_GetLong(blobH, PREFS_SECTION, PREFS_CHANNEL_CACHES, channel_caches, &channel_caches);
	suites.PersistentDataSuite()->AEGP_GetLong(blobH, PREFS_SECTION, PREFS_CACHE_EXPIRATION, cache_timeout, &cache_timeout);
	suites.PersistentDataSuite()->AEGP_GetLong(blobH, PREFS_SECTION, PREFS_AUTO_CACHE, auto_cache_channels, &auto_cache_channels);
	suites.PersistentDataSuite()->AEGP_GetLong(blobH, PREFS_SECTION, PREFS_MEMORY_MAP, memory_map, &memory_map);
	suites.PersistentDataSuite()->AEGP_GetLong(blobH, PREFS_SECTION, PREFS_PERSONAL_INFO, store_personal, &store_personal);
	suites.PersistentDataSuite()->AEGP_GetLong(blobH, PREFS_SECTION, PREFS_MACHINE_INFO, store_machine, &store_machine);
	
	gChannelCaches = channel_caches;
	gCacheTimeout = cache_timeout;
	gAutoCacheChannels = auto_cache_channels;
	gMemoryMap = (memory_map ? TRUE : FALSE);
	gStorePersonal = (store_personal ? TRUE : FALSE);
	gStoreMachine = (store_machine ? TRUE : FALSE);
	
	
	gCachePool.configurePool(gChannelCaches, pica_basicP);
	
	return err;
}


A_Err
OpenEXR_DeathHook(const SPBasicSuite *pica_basicP)
{
	try
	{
		// have to kill all the threads before the DLL is unloaded on windows or
		// AE will hang when the threads are killed by the thread pool
		// destructor being called
		// http://stackoverflow.com/questions/353038/endthreadex0-hangs
		if( IlmThread::supportsThreads() )
			setGlobalThreadCount(0);
		
		
		gCachePool.configurePool(0);
		
		DeleteFileCache(pica_basicP);
		
		
		if(gChannelMap)
			delete gChannelMap;
	}
	catch(...) { return A_Err_PARAMETER; }
	
	return A_Err_NONE;
}


A_Err
OpenEXR_IdleHook(const SPBasicSuite *pica_basicP)
{
	if(gCacheTimeout > 0)
	{
		gCachePool.deleteStaleCaches(gCacheTimeout);
		
		DeleteFileCache(pica_basicP, gCacheTimeout);
	}

	return A_Err_NONE;
}


A_Err
OpenEXR_PurgeHook(const SPBasicSuite *pica_basicP)
{
	gCachePool.configurePool(0);
	gCachePool.configurePool(gChannelCaches);
	
	DeleteFileCache(pica_basicP, 0);
	
	return A_Err_NONE;
}


A_Err
OpenEXR_ConstructModuleInfo(
	AEIO_ModuleInfo *info)
{
	// tell AE all about our plug-in

	A_Err err = A_Err_NONE;
	
	if (info)
	{
		info->sig						=	'oEXR';
		info->max_width					=	2147483647;
		info->max_height				=	2147483647;
		info->num_filetypes				=	1;
		info->num_extensions			=	3;
		info->num_clips					=	0;
		
		info->create_kind.type			=	'EXR ';
		info->create_kind.creator		=	'8BIM';

		info->create_ext.pad			=	'.';
		info->create_ext.extension[0]	=	'e';
		info->create_ext.extension[1]	=	'x';
		info->create_ext.extension[2]	=	'r';
		

		strcpy(info->name, PLUGIN_NAME);
		
		info->num_aux_extensionsS		=	0;

		info->flags						=	AEIO_MFlag_INPUT			| 
											AEIO_MFlag_OUTPUT			| 
											AEIO_MFlag_FILE				|
											AEIO_MFlag_STILL			| 
											AEIO_MFlag_NO_TIME			| 
											AEIO_MFlag_SEQ_OPTIONS_DLG	|
											AEIO_MFlag_SEQUENCE_OPTIONS_OK |
											AEIO_MFlag_HOST_FRAME_START_DIALOG |
											AEIO_MFlag_CAN_DRAW_DEEP	|
											AEIO_MFlag_HAS_AUX_DATA;

		info->flags2					=	AEIO_MFlag2_CAN_DRAW_FLOAT	|
											AEIO_MFlag2_SUPPORTS_ICC_PROFILES;
											
		info->read_kinds[0].mac.type			=	'EXR ';
		info->read_kinds[0].mac.creator			=	AEIO_ANY_CREATOR;
		info->read_kinds[1].ext					=	info->create_ext; // .exr
		info->read_kinds[2].ext.pad				=	'.';
		info->read_kinds[2].ext.extension[0]	=	's';
		info->read_kinds[2].ext.extension[1]	=	'x';
		info->read_kinds[2].ext.extension[2]	=	'r';
		info->read_kinds[3].ext.pad				=	'.';
		info->read_kinds[3].ext.extension[0]	=	'm';
		info->read_kinds[3].ext.extension[1]	=	'x';
		info->read_kinds[3].ext.extension[2]	=	'r';
	}
	else
	{
		err = A_Err_STRUCT;
	}
	return err;
}


A_Err	
OpenEXR_GetInSpecInfo(
	const A_PathType	*file_pathZ,
	OpenEXR_inData		*options,
	AEIO_Verbiage		*verbiageP)
{ 
	A_Err err			=	A_Err_NONE;
	
	//strcpy(verbiageP->name, "some name"); // AE wil insert file name here for us

	strcpy(verbiageP->type, PLUGIN_NAME); // how could we tell a sequence from a file here?
		


	string info("");
	
	if (options) // can't assume there are options, or can we?
	{
		switch(options->compression_type)
		{
			case Imf::NO_COMPRESSION:
				info += "No compression";
				break;

			case Imf::RLE_COMPRESSION:
				info += "RLE compression";
				break;

			case Imf::ZIPS_COMPRESSION:
				info += "Zip compression";
				break;

			case Imf::ZIP_COMPRESSION:
				info += "Zip16 compression";
				break;

			case Imf::PIZ_COMPRESSION:
				info += "Piz compression";
				break;

			case Imf::PXR24_COMPRESSION:
				info += "PXR24 compression";
				break;
			
			case Imf::B44_COMPRESSION:
				info += "B44 compression";
				break;

			case Imf::B44A_COMPRESSION:
				info += "B44A compression";
				break;

			default:
				info += "some weird compression";
				break;
		}
	}
	
		
	if(options && options->real_channels > 4)
	{
		char chan_num[5];
		
		sprintf(chan_num, "%d", options->real_channels);
		
		info += ", ";
		info += chan_num;
		info += " channels: ";
		
		
		A_Boolean first_one = TRUE;
			
		for(int i=0; i < options->real_channels; i++)
		{
			if(first_one)
				first_one = FALSE;
			else
				info += ", ";
			
			//if(options->channels[i].channel_type == PF_ChannelType_DEPTH)
			//	info += "Z-Depth";
			//else if (options->channels[i].channel_type == PF_ChannelType_MOTIONVECTOR;)
			//	info += "Velocity";
			//else
				info += options->channel[i].name;
		}
	}
	
	if(info.length() > AEIO_MAX_MESSAGE_LEN)
	{
		info.resize(AEIO_MAX_MESSAGE_LEN-3);
		info += "...";
	}

	strcpy(verbiageP->sub_type, info.c_str());


	return err;
}


A_Err	
OpenEXR_VerifyFile(
	const A_PathType		*file_pathZ, 
	A_Boolean				*importablePB)
{
	A_Err err			=	A_Err_NONE;
 
	try {
		IStreamPlatform instream(file_pathZ);

		char bytes[4];
		instream.read(bytes, sizeof(bytes));

		*importablePB = isImfMagic(bytes);
	}
	catch(...){ *importablePB = FALSE; }
	
	return err;
}


// layer.R -> R
static string channelName(const string &full_channel_name)
{
	size_t pos = full_channel_name.rfind('.');
	
	if(pos != string::npos && pos != 0 && pos + 1 < full_channel_name.size())
	{
		return full_channel_name.substr(pos + 1);
	}
	else
		return full_channel_name;
}

static bool channelOrderCompare(const string &first, const string &second)
{
	static const char * const channel_list[] = {"A", "a", "alpha",
												"R", "r", "red",
												"G", "g", "green",
												"B", "b", "blue" };
							
	int first_rank = 12,
		second_rank = 12;
		
	string first_channel = channelName(first);
	string second_channel = channelName(second);
	
	for(int i=0; i < 12; i++)
	{
		if(first_channel == channel_list[i])
			first_rank = i;
			
		if(second_channel == channel_list[i])
			second_rank = i;
	}
	
	if(first_rank == second_rank)
	{
		return (strcmp(first.c_str(), second.c_str()) <= 0);
	}
	else
	{
		return (first_rank < second_rank);
	}
}


static void
ResizeOptionsHandle(
	AEIO_BasicData		*basic_dataP,
	AEIO_Handle			optionsH,
	OpenEXR_inData		**options,
	A_u_long			num_channels)
{
	AEGP_SuiteHandler suites(basic_dataP->pica_basicP);
	
	AEGP_MemSize mem_size = sizeof(OpenEXR_inData) + (sizeof(PF_ChannelDesc) * (max<A_u_long>(num_channels, 1) - 1));
	
	suites.MemorySuite()->AEGP_UnlockMemHandle(optionsH);
	
	suites.MemorySuite()->AEGP_ResizeMemHandle("Old Handle Resize", mem_size, optionsH);
	
	suites.MemorySuite()->AEGP_LockMemHandle(optionsH, (void**)options);
}


A_Err
OpenEXR_FileInfo(
	AEIO_BasicData		*basic_dataP,
	const A_PathType	*file_pathZ,
	FrameSeq_Info		*info,
	AEIO_Handle			optionsH)
{
	// read a file and pass AE the basic info
	
	A_Err err						=	A_Err_NONE;
	
	try{
	
	OpenEXR_inData *options = NULL;
	
	
	// we're going to increase the options handle size as needed and then shrink it at the end
	// will start with and increment by 64
	A_u_long options_size = 64;
	
	ResizeOptionsHandle(basic_dataP, optionsH, &options, options_size);
	
	
	// read the EXR
	IStreamPlatform instream(file_pathZ);
	HybridInputFile in(instream);
	
	const Header &head = in.header(0);
	const ChannelList &channels = in.channels();


	// what kind of image is this?
	if( head.channels().findChannel("A") )
		info->planes = 4; // has to be RGBA with an alpha
	else if( channels.findChannel("Y") &&
			!channels.findChannel("RY") &&
			!channels.findChannel("BY") &&
			!channels.findChannel("R") &&
			!channels.findChannel("G") &&
			!channels.findChannel("B") )
		info->planes = 1; // gray
	else
		info->planes = 3;

	// says in the EXR spec to use premultiplied (p5 of TechnicalIntroduction.pdf) 
	// we can force it programatically here or put this line in interpretation rules.txt
	// *, *, *, "oEXR", * ~ *, *, *, P, *, *
	if(info->planes == 4)
		info->alpha_type = AEIO_Alpha_PREMUL; 
		
	
	// EXR RGB channels are always float
	info->depth = 32;

	
	// EXR stores pixel aspect ratio as a floating point number (boo!)
	float pixel_aspect_ratio = head.pixelAspectRatio();
	
	// try to be smart making this into a rational scale
	if(pixel_aspect_ratio == 1.0f)
		info->pixel_aspect_ratio.num = info->pixel_aspect_ratio.den = 1; // probably could do nothing, actually
	else
	{
		info->pixel_aspect_ratio.den = 1000;
		info->pixel_aspect_ratio.num = (pixel_aspect_ratio * (float)info->pixel_aspect_ratio.den) + 0.5f;
	}
	
	// if I stored a ratio in the file, let's use that
#define PIXEL_ASPECT_RATIONAL_KEY	"pixelAspectRatioRational"
	const RationalAttribute *hsf_ratio = head.findTypedAttribute<RationalAttribute>(PIXEL_ASPECT_RATIONAL_KEY);
	
	if(hsf_ratio)
	{
		info->pixel_aspect_ratio.num = hsf_ratio->value().n;
		info->pixel_aspect_ratio.den = hsf_ratio->value().d;
	}
	
	
	if( hasFramesPerSecond(head) )
		info->framerate = framesPerSecond(head);
	
	
	// got chromaticities? or a profile?
	if( hasICCprofile(head) )
	{
		const iccProfileAttribute &attr = ICCprofileAttribute(head);
		
		size_t len = 0;
		const void *prof = attr.value(len);
		
		info->icc_profile = malloc(len);
		memcpy(info->icc_profile, prof, len);
		info->icc_profile_len = len;
	}
	
	if( hasChromaticities(head) && info->chromaticities )
	{
		const Chromaticities &chrm = chromaticities(head);
		
		info->chromaticities->red.x = chrm.red.x;		info->chromaticities->red.y = chrm.red.y;
		info->chromaticities->green.x = chrm.green.x;	info->chromaticities->green.y = chrm.green.y;
		info->chromaticities->blue.x = chrm.blue.x;		info->chromaticities->blue.y = chrm.blue.y;
		info->chromaticities->white.x = chrm.white.x;	info->chromaticities->white.y = chrm.white.y;
		
		// check for a color space name too
	#define COLOR_SPACE_NAME_KEY "chromaticitiesName"	
		const StringAttribute *chrom_name = head.findTypedAttribute<StringAttribute>(COLOR_SPACE_NAME_KEY);
		
		if(chrom_name)
			strncpy(info->chromaticities->color_space_name, chrom_name->value().c_str(), CHROMATICITIES_MAX_NAME_LEN);
	}

	
	// for our sub-type
	options->compression_type = head.compression();
	
	
	// channels
	A_long channel_index = 0;
	
	
	for(ChannelList::ConstIterator i = channels.begin(); i != channels.end(); ++i)
	{
		// things we'll want to know about the channels
		string channel_name(i.name());
		
		const Channel &channel = i.channel();
		
		Imf::PixelType pix_type = channel.type;

		// generic channel defninitions
		if(channel_name.size() <= PF_CHANNEL_NAME_LEN)
		{
			options->channel[channel_index].channel_type = PF_ChannelType_UNKNOWN;
			options->channel[channel_index].dimension = 1;
			strcpy(options->channel[channel_index].name, channel_name.c_str());
			
			options->channel[channel_index].data_type = (pix_type == Imf::HALF) ? PF_DataType_FLOAT :
														(pix_type == Imf::FLOAT) ? PF_DataType_FLOAT :
														(pix_type == Imf::UINT) ? PF_DataType_LONG :
														PF_DataType_CHAR; //ummmm?
														
			channel_index++;
			
			// resize options handle if necessary
			if(options_size == channel_index)
			{
				options_size += 64;
				
				ResizeOptionsHandle(basic_dataP, optionsH, &options, options_size);
			}
		}
		
	}
	
	options->real_channels = channel_index;
   
	
	// channel mapping
	if(gChannelMap)
	{
		int regular_channels = channel_index;
		
		for(int i=0; i < regular_channels; i++)
		{
			ChannelEntry entry;
			
			if( gChannelMap->findEntry(options->channel[i].name, entry) )
			{
				if( entry.dimensions() > 1 )
				{
					// multi-dimensional channels
					bool have_them = true;
					
					// do we really have all the relevant channels?
					for(int c=0; c < entry.dimensions(); c++)
						if( !head.channels().findChannel( entry.chan_part(c).c_str() ) )
							have_them = false;
					
					if(have_them && entry.name().size() < PF_CHANNEL_NAME_LEN)
					{
						// make a new muti-dimensional channel
						options->channel[channel_index].channel_type = entry.type();
						options->channel[channel_index].data_type = entry.data();
						options->channel[channel_index].dimension = entry.dimensions();
						strcpy(options->channel[channel_index].name, entry.name().c_str());
						
						channel_index++;

						// resize options handle if necessary
						if(options_size == channel_index)
						{
							options_size += 64;
							
							ResizeOptionsHandle(basic_dataP, optionsH, &options, options_size);
						}
					}
				}
				else if(entry.dimensions() == 1)
				{
					// single dimension - we just label the existing channel
					options->channel[i].channel_type = entry.type();
					options->channel[i].data_type = entry.data();
				}
			}
		}
	}
	
	
	// create multi-dimensional channel layers that can be loaded with one DrawAuxChannel call
	set<string> layerNames;
	channels.layers(layerNames);
	
	for(set<string>::iterator i = layerNames.begin(); i != layerNames.end(); i++)
	{
		// put into a list so we can use the sort function
		ChannelList::ConstIterator f, l;
		channels.channelsInLayer(*i, f, l);
		
		list<string> channels_in_layer;
		
		for(ChannelList::ConstIterator j = f; j != l; j++)
			channels_in_layer.push_back(j.name());
		
		if(channels_in_layer.size() > 1)
		{
			// sort into RGBA
			channels_in_layer.sort(channelOrderCompare);
			
			// can we use the short form of a compound string?
			// so instead of layer.R|layer.G|layer.B we do layer.RGB
			// this increases the chance that we can fit the comound name into the channel name
			// really, the only important thing is that that compound name will be unambiguous
			bool short_form = true;
			
			for(list<string>::const_iterator j = channels_in_layer.begin(); j != channels_in_layer.end() && short_form; j++)
			{
				const string &ref_channel = channels_in_layer.front();
				
				if( j->find_last_of('.') == string::npos ||
					j->find_last_of('.') == (j->size() - 1) ||
					j->find_last_of('.') != ref_channel.find_last_of('.') ||
					j->substr(0, j->find_last_of('.')) != ref_channel.substr(0, ref_channel.find_last_of('.')) )
				{
					short_form = false;
				}
			}
			
			// transfer from list to compound string
			string compound_layer_name;
			Imf::PixelType pix_type;
			int dimension = 0;
			
			for(list<string>::const_iterator j = channels_in_layer.begin(); j != channels_in_layer.end() && dimension < 4; j++)
			{
				if( compound_layer_name.empty() )
				{
					// set type based on first channel
					pix_type = channels[*j].type;
					
					compound_layer_name = *j;
					
					dimension++;
				}
				else if(channels[*j].type == pix_type)
				{
					if(short_form)
					{
						compound_layer_name += j->substr(j->find_last_of('.') + 1);
					}
					else
					{
						compound_layer_name += "|" + *j;
					}
					
					dimension++;
				}
			}
			
			if(compound_layer_name.size() <= PF_CHANNEL_NAME_LEN)
			{
				options->channel[channel_index].channel_type = PF_ChannelType_UNKNOWN;
				options->channel[channel_index].dimension = dimension;
				strcpy(options->channel[channel_index].name, compound_layer_name.c_str());
				
				options->channel[channel_index].data_type = (pix_type == Imf::HALF) ? PF_DataType_FLOAT :
															(pix_type == Imf::FLOAT) ? PF_DataType_FLOAT :
															(pix_type == Imf::UINT) ? PF_DataType_LONG :
															PF_DataType_CHAR; //ummmm?
															
				channel_index++;

				// resize options handle if necessary
				if(options_size == channel_index)
				{
					options_size += 64;
					
					ResizeOptionsHandle(basic_dataP, optionsH, &options, options_size);
				}
			}
		}
	}
	
	
	options->channels = channel_index;
	
	
	// shrink options handle back down to size we actually need
	ResizeOptionsHandle(basic_dataP, optionsH, &options, options->channels);
	
	
	
	const Box2i &dataW = in.dataWindow();
	const Box2i &dispW = in.displayWindow();
	
	// if conditions are right, pop up our displayWindow dialog
	// only when the footage is first imported
	if(options->display_window == DW_UNKNOWN)
	{
		AEGP_SuiteHandler suites(basic_dataP->pica_basicP);
						
		AEGP_PersistentBlobH blobH = NULL;
		suites.PersistentDataSuite()->AEGP_GetApplicationBlob(&blobH);
		
	#define PREFS_DISPLAYWINDOW "displayWindow Default"
		A_long displayWindowDefaultL = TRUE;
		suites.PersistentDataSuite()->AEGP_GetLong(blobH, PREFS_SECTION, PREFS_DISPLAYWINDOW, displayWindowDefaultL, &displayWindowDefaultL);
			
		options->display_window = (displayWindowDefaultL ? DW_DISPLAY_WINDOW : DW_DATA_WINDOW);
		
		
		const bool force_dlog = ShiftKeyHeld();
		
		if(dataW != dispW || force_dlog)
		{
			A_Boolean ui_is_suppressedB = FALSE;
			suites.UtilitySuite()->AEGP_GetSuppressInteractiveUI(&ui_is_suppressedB);
			
		#define PREFS_AUTO_DISPLAYWINDOW "Automatically show displayWindow dialog"
			A_long autoDisplayWindowL = FALSE;
			suites.PersistentDataSuite()->AEGP_GetLong(blobH, PREFS_SECTION, PREFS_AUTO_DISPLAYWINDOW, autoDisplayWindowL, &autoDisplayWindowL);
		
		
			if(!ui_is_suppressedB && (autoDisplayWindowL || force_dlog))
			{
				stringstream message, dispWstr, dataWstr;
				
				if(dataW != dispW)
				{
					message << "displayWindow and dataWindow are different. " <<
						"Choose one to determine file size.";
				}
				else
				{
					message << "Shift key was held down. Choose to use displayWindow or dataWindow.";
				}
				
				const int displayWidth = dispW.max.x - dispW.min.x + 1;
				const int displayHeight = dispW.max.y - dispW.min.y + 1;
				dispWstr << "displayWindow (" << displayWidth << "x" << displayHeight << ")";
				
				const int dataWidth = dataW.max.x - dataW.min.x + 1;
				const int dataHeight = dataW.max.y - dataW.min.y + 1;
				dataWstr << "dataWindow (" << dataWidth << "x" << dataHeight << ")";
				

				A_Boolean displayWindowB = displayWindowDefaultL;
				A_Boolean displayWindowDefaultB = displayWindowDefaultL;
				A_Boolean autoShowDialogB = autoDisplayWindowL;
				A_Boolean clicked_ok = FALSE;
			
				OpenEXR_displayWindowDialog(basic_dataP,
											message.str().c_str(),
											dispWstr.str().c_str(),
											dataWstr.str().c_str(),
											&displayWindowB,
											&displayWindowDefaultB,
											&autoShowDialogB,
											&clicked_ok);
											
				
				if(displayWindowDefaultL != displayWindowDefaultB)
				{
					displayWindowDefaultL = displayWindowDefaultB;
				
					suites.PersistentDataSuite()->AEGP_SetLong(blobH, PREFS_SECTION, PREFS_DISPLAYWINDOW, displayWindowDefaultL);
				}
				
				if(autoDisplayWindowL != autoShowDialogB)
				{
					autoDisplayWindowL = autoShowDialogB;
						
					suites.PersistentDataSuite()->AEGP_SetLong(blobH, PREFS_SECTION, PREFS_AUTO_DISPLAYWINDOW, autoDisplayWindowL);
				}
				
				if(clicked_ok)
				{
					options->display_window = (displayWindowB ? DW_DISPLAY_WINDOW : DW_DATA_WINDOW);
				}
				else
					return AEIO_Err_USER_CANCEL;
			}
		}
		
		// turn on caching if we exceed the auto-cache threshold
		if(gAutoCacheChannels > 0 && options->real_channels >= gAutoCacheChannels)
			options->cache_channels = TRUE;
	}
	
	
	const Box2i &sizeW = (options->display_window == DW_DATA_WINDOW ? dataW : dispW);

	info->width = sizeW.max.x - sizeW.min.x + 1;
	info->height = sizeW.max.y - sizeW.min.y + 1;
	
	
	// this is how you'd iterate through file attributes if you were so inclined
	//for (Header::ConstIterator j = head.begin(); j != head.end(); ++j)
	//{
	//	const Attribute &attrib = j.attribute();
	//	
	//	string attr_type(attrib.typeName());
	//	
	//	string attr_name(j.name());
	//}


	}catch(...) { err = AEIO_Err_PARSING; }
	
	return err;
}


static void
RefreshChannels(
	AEIO_BasicData		*basic_dataP,
	AEIO_Handle			optionsH,
	const A_PathType	*file_pathZ)
{
	// run through FileInfo process again to fill in out channel info

	// here's a fake FrameSeq_Info
	FrameSeq_Info info;
	
	PF_Pixel	premult_color = {255, 0, 0, 0};
	FIEL_Label	field_label;

	info.premult_color	= &premult_color;
	info.field_label	= &field_label;
	
	info.icc_profile	= NULL;
	info.chromaticities = NULL;
	
	
	OpenEXR_FileInfo(basic_dataP, file_pathZ, &info, optionsH);
	
	
	if(info.icc_profile)
		free(info.icc_profile);
}


typedef struct {
	const AEIO_InterruptFuncs *interP;
	void *in;
	size_t in_rowbytes;
	void *out;
	size_t out_rowbytes;
	int num_channels;
	int width;
	PF_Point scale;
	PF_Point shift;
} IterateData;

template <typename InFormat, typename OutFormat>
static A_Err CopyBufferIterate( void	*refconPV,
								A_long	thread_indexL,
								A_long	i,
								A_long	iterationsL)
{
	A_Err err = A_Err_NONE;
	
	IterateData *i_data = (IterateData *)refconPV;
	
	InFormat *in_pix = (InFormat *)((char *)i_data->in + (i * i_data->in_rowbytes * i_data->scale.v) + (i_data->in_rowbytes * i_data->shift.v) + (i_data->num_channels * sizeof(InFormat) * i_data->shift.h));
	OutFormat *out_pix = (OutFormat *)((char *)i_data->out + (i * i_data->out_rowbytes));
	
	for(int x=0; x < i_data->width; x++)
	{
		for(int i=0; i < i_data->num_channels; i++)
		{
			*out_pix++ = *in_pix++;
		}
		
		in_pix += (i_data->scale.v - 1) * i_data->num_channels;
	}

#ifdef NDEBUG
	if(thread_indexL == 0 && i_data->interP && i_data->interP->abort0)
		err = i_data->interP->abort0(i_data->interP->refcon);
#endif

	return err;
}


typedef struct RgbaPixel {
	half red;
	half green;
	half blue;
	half alpha;
} RgbaPixel;

typedef struct {
	const AEIO_InterruptFuncs *interP;
	const void *in;
	size_t in_rowbytes;
	void *out;
	size_t out_rowbytes;
	int width;
	bool has_alpha;
} RgbaIterateData;

template <typename InFormat, typename OutFormat>
static A_Err CopyRgbaBufferIterate( void	*refconPV,
									A_long	thread_indexL,
									A_long	i,
									A_long	iterationsL)
{
	A_Err err = A_Err_NONE;
	
	RgbaIterateData *i_data = (RgbaIterateData *)refconPV;
	
	InFormat *in_pix = (InFormat *)((char *)i_data->in + (i * i_data->in_rowbytes));
	OutFormat *out_pix = (OutFormat *)((char *)i_data->out + (i * i_data->out_rowbytes));
	
	for(int x=0; x < i_data->width; x++)
	{
		out_pix->red = in_pix->red;
		out_pix->green = in_pix->green;
		out_pix->blue = in_pix->blue;
		
		if(i_data->has_alpha)
			out_pix->alpha = in_pix->alpha;
		else
			out_pix->alpha = 1.f;
		
		in_pix++;
		out_pix++;
	}

#ifdef NDEBUG
	if(thread_indexL == 0 && i_data->interP && i_data->interP->abort0)
		err = i_data->interP->abort0(i_data->interP->refcon);
#endif

	return err;
}


A_Err	
OpenEXR_DrawSparseFrame(
	AEIO_BasicData					*basic_dataP,
	const AEIO_DrawSparseFramePB	*sparse_framePPB, 
	PF_EffectWorld					*wP,
	AEIO_DrawingFlags				*draw_flagsP,
	const A_PathType				*file_pathZ,
	FrameSeq_Info					*info,
	OpenEXR_inData					*options)
{ 
	// read pixels into the float buffer

	A_Err	err		= A_Err_NONE,
			err2	= A_Err_NONE;
	
	AEGP_SuiteHandler suites(basic_dataP->pica_basicP);
	
	PF_EffectWorld temp_world_data;
	PF_EffectWorld *temp_world = NULL;
	
	AEIO_Handle temp_RgbaH = NULL;
	
	
	try{
	
	IStreamPlatform instream(file_pathZ, basic_dataP->pica_basicP);

	if(gMemoryMap)
		instream.memoryMap();

	HybridInputFile in(instream);
	
	const Header &head = in.header(0);
	
	
	if(options == NULL)
		throw NullExc("Why are options NULL?");
	
	
	PF_EffectWorld *active_world = wP;
	PF_PixelPtr pixel_origin = active_world->data;
	
	
	const Box2i &dataW = head.dataWindow();
	const Box2i &dispW = head.displayWindow();
	
	
	if(options->display_window == DW_DISPLAY_WINDOW)
	{
		if(	in.parts() > 1 ||
			(dataW.min.x > dispW.min.x) ||
			(dataW.min.y > dispW.min.y) ||
			(dataW.max.x < dispW.max.x) ||
			(dataW.max.y < dispW.max.y) )
		{
			// if some pixels will not be written to,
			// clear the destination world out first
			PF_PixelFloat clear = { 0.f, 0.f, 0.f, 0.f };
			
			if(info->planes < 4)
				clear.alpha = 1.f;
			
			suites.PFFillMatteSuite()->fill_float(NULL, &clear, NULL, wP);
			
			
			// if the dataWindow does not actually intersect the displayWindow,
			// no need to continue further
			if( !dataW.intersects(dispW) )
			{
				return err;
			}
		}
	
		const int display_width = (dispW.max.x - dispW.min.x) + 1;
		const int display_height = (dispW.max.y - dispW.min.y) + 1;
		
		assert(display_width == wP->width);
		assert(display_height == wP->height);
		
		// if dataWindow is completely inside displayWindow, we can use the
		// existing PF_World, otherwise have to create a new one
		if( (dataW.min.x >= dispW.min.x) &&
			(dataW.min.y >= dispW.min.y) &&
			(dataW.max.x <= dispW.max.x) &&
			(dataW.max.y <= dispW.max.y) )
		{
			active_world = wP;
			
			pixel_origin = (PF_PixelPtr)((char *)active_world->data +
											(active_world->rowbytes * (dataW.min.y - dispW.min.y)) +
											(sizeof(PF_Pixel32) * (dataW.min.x - dispW.min.x)) );
		}
		else
		{
			active_world = temp_world = &temp_world_data;
			
			const int data_width = (dataW.max.x - dataW.min.x) + 1;
			const int data_height = (dataW.max.y - dataW.min.y) + 1;
			
			err = suites.PFWorldSuite()->PF_NewWorld(NULL, data_width, data_height, TRUE,
													PF_PixelFormat_ARGB128, temp_world);
													
			if(err)
				throw BaseExc("Was not able to make a PFWorld");
													
			pixel_origin = active_world->data;
		}
	}
	else
	{
		assert(options->display_window == DW_DATA_WINDOW);
		
		const int data_width = (dataW.max.x - dataW.min.x) + 1;
		const int data_height = (dataW.max.y - dataW.min.y) + 1;
		
		assert(data_width == wP->width);
		assert(data_height == wP->height);
		
		active_world = wP;
		pixel_origin = active_world->data;
	}
	

	// these will return true as long as the user hasn't interrupted
#ifdef NDEBUG
	#define CONT()	( (sparse_framePPB && sparse_framePPB->inter.abort0) ? !(err2 = sparse_framePPB->inter.abort0(sparse_framePPB->inter.refcon) ) : TRUE)
	#define PROG(COUNT, TOTAL)	( (sparse_framePPB && sparse_framePPB->inter.progress0) ? !(err2 = sparse_framePPB->inter.progress0(sparse_framePPB->inter.refcon, COUNT, TOTAL) ) : TRUE)
	const AEIO_InterruptFuncs *inter = (sparse_framePPB ? &sparse_framePPB->inter : NULL);
#else
	#define CONT()					TRUE
	#define PROG(COUNT, TOTAL)		TRUE
	const AEIO_InterruptFuncs *inter = NULL;
#endif

	// Since we're reading into an RGBA float buffer, we can usually use the general read function,
	// which handles half->float conversion for us.
	// YCC and other cases use the more robust RgbaInputFile.
	if( head.channels().findChannel("R") &&
		head.channels().findChannel("G") &&
		head.channels().findChannel("B") )
	{
		FrameBuffer frameBuffer;
		
		char *exr_ARGB_origin = (char *)pixel_origin - (sizeof(PF_Pixel32) * dataW.min.x) - (active_world->rowbytes * dataW.min.y);
		
		frameBuffer.insert("A", Slice(Imf::FLOAT, exr_ARGB_origin + (sizeof(PF_FpShort) * 0), sizeof(PF_FpShort) * 4, active_world->rowbytes, 1, 1, 1.0) );
		frameBuffer.insert("R", Slice(Imf::FLOAT, exr_ARGB_origin + (sizeof(PF_FpShort) * 1), sizeof(PF_FpShort) * 4, active_world->rowbytes, 1, 1, 0.0) );
		frameBuffer.insert("G", Slice(Imf::FLOAT, exr_ARGB_origin + (sizeof(PF_FpShort) * 2), sizeof(PF_FpShort) * 4, active_world->rowbytes, 1, 1, 0.0) );
		frameBuffer.insert("B", Slice(Imf::FLOAT, exr_ARGB_origin + (sizeof(PF_FpShort) * 3), sizeof(PF_FpShort) * 4, active_world->rowbytes, 1, 1, 0.0) );

		
		OpenEXR_ChannelCache *chan_cache = gCachePool.findCache(instream);
		
		if(chan_cache == NULL && options->cache_channels && gChannelCaches > 0)
		{
			chan_cache = gCachePool.addCache(in, instream, inter);			
		}
		
		if(chan_cache)
		{
			if( CONT() )
			{
				chan_cache->fillFrameBuffer(frameBuffer, dataW);
			}
		}
		else
		{
			in.setFrameBuffer(frameBuffer);

			int begin_line = dataW.min.y;
			int end_line = dataW.max.y;
			
			if(options->display_window == DW_DISPLAY_WINDOW)
			{
				begin_line = max(dataW.min.y, dispW.min.y);
				end_line = min(dataW.max.y, dispW.max.y);
			}
			
			const int scanline_block_size = ScanlineBlockSize(in);
			
			int y = begin_line;
			
			while(y <= end_line && PROG(y - begin_line, end_line - begin_line) )
			{
				int high_scanline = min(y + scanline_block_size - 1, end_line);
				
				in.readPixels(y, high_scanline);
				
				y = high_scanline + 1;
			}
		}
	}
	else
	{
		const int scanline_block_size = ScanlineBlockSize(in);
		
		// use this more robust RGBA object when things are dicey
		instream.seekg(0);
		
		RgbaInputFile inputFile(instream);

		
		const int data_width = (dataW.max.x - dataW.min.x) + 1;
		const int data_height = (dataW.max.y - dataW.min.y) + 1;
		
		
		const size_t data_rowbytes = sizeof(RgbaPixel) * data_width;
		
		suites.MemorySuite()->AEGP_NewMemHandle( S_mem_id, "temp_RgbaH",
												data_rowbytes * data_height,
												AEGP_MemFlag_CLEAR, &temp_RgbaH);
												
		if(temp_RgbaH == NULL)
			throw NullExc("temp_RgbaH is NULL");
		
		
		char *temp_Rgba = NULL;
		
		suites.MemorySuite()->AEGP_LockMemHandle(temp_RgbaH, (void**)&temp_Rgba);
		
		if(temp_Rgba == NULL)
			throw NullExc("temp_Rgba is NULL");
		
		
		Rgba *Rgba_origin = (Rgba *)(temp_Rgba - (sizeof(Rgba) * dataW.min.x) - (data_rowbytes * dataW.min.y));
		
		inputFile.setFrameBuffer(Rgba_origin, 1, data_width);
		
		
		int begin_line = dataW.min.y;
		int end_line = dataW.max.y;
		
		if(options->display_window == DW_DISPLAY_WINDOW)
		{
			begin_line = max(dataW.min.y, dispW.min.y);
			end_line = min(dataW.max.y, dispW.max.y);
		}
		
		
		int y = begin_line;
		
		while(y <= end_line && PROG(y - begin_line, end_line - begin_line))
		{
			int high_scanline = min(y + scanline_block_size - 1, end_line);
			
			inputFile.readPixels(y, high_scanline);
			
			y = high_scanline + 1;
		}
		
		
		bool have_alpha = (inputFile.channels() & WRITE_A);
		
		RgbaIterateData i_data = { inter, temp_Rgba, data_rowbytes, pixel_origin, active_world->rowbytes, data_width, have_alpha };
		
		err2 = suites.AEGPIterateSuite()->AEGP_IterateGeneric(data_height, (void *)&i_data, CopyRgbaBufferIterate<RgbaPixel, PF_PixelFloat>);
	}
	
	
	if(temp_world && !err && !err2)
	{
		// have to draw dataWindow pixels inside the displayWindow
		PF_EffectWorld *display_world = wP;
		PF_EffectWorld *data_world = temp_world;
		
		
		PF_Point disp_origin, data_origin;
		
		if(dispW.min.x < dataW.min.x)
		{
			disp_origin.h = dataW.min.x - dispW.min.x;
			data_origin.h = 0;
		}
		else
		{
			disp_origin.h = 0;
			data_origin.h = dispW.min.x - dataW.min.x;
		}
		
		
		if(dispW.min.y < dataW.min.y)
		{
			disp_origin.v = dataW.min.y - dispW.min.y;
			data_origin.v = 0;
		}
		else
		{
			disp_origin.v = 0;
			data_origin.v = dispW.min.y - dataW.min.y;
		}
		
		PF_PixelPtr display_pixel_origin = (PF_PixelPtr)((char *)display_world->data +
														(disp_origin.v * display_world->rowbytes) +
														(disp_origin.h * sizeof(PF_PixelFloat)) );

		PF_PixelPtr data_pixel_origin = (PF_PixelPtr)((char *)data_world->data +
														(data_origin.v * data_world->rowbytes) +
														(data_origin.h * sizeof(PF_PixelFloat)) );
		
		
		int copy_width = min(dispW.max.x, dataW.max.x) - max(dispW.min.x, dataW.min.x) + 1;
		int copy_height = min(dispW.max.y, dataW.max.y) - max(dispW.min.y, dataW.min.y) + 1;
		

		PF_Point scale = {1, 1};
		PF_Point shift = {0, 0};
		
		IterateData i_data = { inter, (void *)data_pixel_origin, data_world->rowbytes, (void *)display_pixel_origin, display_world->rowbytes, 4, copy_width, scale, shift };
	
		err2 = suites.AEGPIterateSuite()->AEGP_IterateGeneric(copy_height, (void *)&i_data, CopyBufferIterate<float, float>);
	}

	
	}
	catch(IoExc) {}
	catch(InputExc) {}
	catch(CancelExc &e) { err2 = e.err(); }
	catch(...) { err = AEIO_Err_PARSING; }


	if(temp_world)
		suites.PFWorldSuite()->PF_DisposeWorld(NULL, temp_world);
	
	
	if(temp_RgbaH)
		suites.MemorySuite()->AEGP_FreeMemHandle(temp_RgbaH);
		
	
	if(err2) { err = err2; }
	
	return err;
}


A_Err	
OpenEXR_InitInOptions(
	AEIO_BasicData	*basic_dataP,
	OpenEXR_inData	*options)
{
	// initialize the options when they're first created
	A_Err err = A_Err_NONE;
	
	options->compression_type = Imf::NUM_COMPRESSION_METHODS + 1; // so basically unknown
	options->cache_channels = FALSE;
	options->display_window = DW_UNKNOWN;
	
	options->real_channels = options->channels = 0; // until they get filled in

	return err;
}


A_Err	
OpenEXR_CopyInOptions(
	AEIO_BasicData	*basic_dataP,
	OpenEXR_inData	*new_options,
	const OpenEXR_inData	*old_options)
{
	// test to see if this looks like one of our options handles
	if(old_options->compression_type <= Imf::B44A_COMPRESSION &&
		(old_options->cache_channels == TRUE || old_options->cache_channels == FALSE) &&
		old_options->display_window <= DW_UNKNOWN)
	{
		new_options->cache_channels = old_options->cache_channels;
		new_options->display_window = old_options->display_window;
	}
	
	return A_Err_NONE;
}


A_Err	
OpenEXR_ReadOptionsDialog(
	AEIO_BasicData	*basic_dataP,
	OpenEXR_inData	*options,
	A_Boolean		*user_interactedPB0)
{
	A_Err err = A_Err_NONE;
	
	try{
	
	if(options)
	{
		A_Boolean interacted = FALSE;
		
		A_Boolean cache = options->cache_channels;
		A_long num_caches = gChannelCaches;
		
		
		err = OpenEXR_InDialog(basic_dataP, &cache, &num_caches, &interacted);
		
		
		if(interacted)
		{
			options->cache_channels = cache;
			
			if(num_caches != gChannelCaches)
			{
				AEGP_SuiteHandler suites(basic_dataP->pica_basicP);
							
				AEGP_PersistentBlobH blobH = NULL;
				suites.PersistentDataSuite()->AEGP_GetApplicationBlob(&blobH);
	
				suites.PersistentDataSuite()->AEGP_SetLong(blobH, PREFS_SECTION, PREFS_CHANNEL_CACHES, num_caches);

				gChannelCaches = num_caches;
				
				gCachePool.configurePool(gChannelCaches, basic_dataP->pica_basicP);
			}
			
			if(user_interactedPB0) // old AE was leaving this NULL
				*user_interactedPB0 = TRUE;
		}
	}
	
	}catch(...) { err = AEIO_Err_INCONSISTENT_PARAMETERS; }
	
	return err;
}


// for backward compatibility, we have to save options handle this size in the project
// long story, but it has to do with inflate not getting a legit sequence path
typedef struct Old_OpenEXR_inData
{
	A_u_char			compression_type;
	A_long				channels;
	PF_ChannelDesc		channel[128];
} Old_OpenEXR_inData;

A_Err
OpenEXR_FlattenInputOptions(
	AEIO_BasicData	*basic_dataP,
	AEIO_Handle		optionsH)
{
	AEGP_SuiteHandler suites(basic_dataP->pica_basicP);
	
	suites.MemorySuite()->AEGP_ResizeMemHandle("Flattening Options", sizeof(Old_OpenEXR_inData), optionsH);

	OpenEXR_inData *options = NULL;
	suites.MemorySuite()->AEGP_LockMemHandle(optionsH, (void **)&options);
	
	if(options)
	{
		options->real_channels = options->channels = 0;
		
		suites.MemorySuite()->AEGP_UnlockMemHandle(optionsH);
	}
	
	return A_Err_NONE;
}

A_Err
OpenEXR_InflateInputOptions(
	AEIO_BasicData	*basic_dataP,
	AEIO_Handle		optionsH)
{
	// the key is we set the channel numbers to 0 so they get forced to reload
	AEGP_SuiteHandler suites(basic_dataP->pica_basicP);
	
	suites.MemorySuite()->AEGP_ResizeMemHandle("Flattening Options", sizeof(OpenEXR_inData), optionsH);

	OpenEXR_inData *options = NULL;
	suites.MemorySuite()->AEGP_LockMemHandle(optionsH, (void **)&options);
	
	if(options)
	{
		options->real_channels = options->channels = 0;
		
		suites.MemorySuite()->AEGP_UnlockMemHandle(optionsH);
	}

	return A_Err_NONE;
}


#pragma mark-


A_Err	
OpenEXR_GetNumAuxChannels(
	AEIO_BasicData		*basic_dataP,
	AEIO_Handle			optionsH,
	const A_PathType	*file_pathZ,
	A_long				*num_channelsPL)
{
	A_Err err						=	A_Err_NONE;
	
	AEGP_SuiteHandler suites(basic_dataP->pica_basicP);
	
	
	OpenEXR_inData *options = NULL;
	
	suites.MemorySuite()->AEGP_LockMemHandle(optionsH, (void**)&options);
	

	// might have to get this info from the file again
	if(options->channels == 0) // should always have at least one
	{
		RefreshChannels(basic_dataP, optionsH, file_pathZ);
		
		suites.MemorySuite()->AEGP_LockMemHandle(optionsH, (void**)&options);
	}


	*num_channelsPL = options->channels;


	suites.MemorySuite()->AEGP_UnlockMemHandle(optionsH);

	return err;
}

									
A_Err	
OpenEXR_GetAuxChannelDesc(
	AEIO_BasicData		*basic_dataP,
	AEIO_Handle			optionsH,
	const A_PathType	*file_pathZ,
	A_long				chan_indexL,
	PF_ChannelDesc		*descP)
{
	A_Err err						=	A_Err_NONE;

	AEGP_SuiteHandler suites(basic_dataP->pica_basicP);
	
	
	OpenEXR_inData *options = NULL;
	
	suites.MemorySuite()->AEGP_LockMemHandle(optionsH, (void**)&options);


	// might have to get this info from the file again
	if(options->channels == 0) // should always have at least one
	{
		suites.MemorySuite()->AEGP_UnlockMemHandle(optionsH);

		RefreshChannels(basic_dataP, optionsH, file_pathZ);
		
		suites.MemorySuite()->AEGP_LockMemHandle(optionsH, (void**)&options);
	}


	// just copy the appropriate ChannelDesc
	*descP = options->channel[chan_indexL];


	suites.MemorySuite()->AEGP_UnlockMemHandle(optionsH);

	return err;
}


// rarely you might get a channel that has a subsampling factor (mostly if you foolishly grab a Y/C channel)
// this will expand those channels to the full buffer size
static void FixSubsampling(const FrameBuffer &framebuffer, const Box2i &dw)
{
	for(FrameBuffer::ConstIterator i = framebuffer.begin(); i != framebuffer.end(); i++)
	{
		const Slice &slice = i.slice();
		
		if(slice.xSampling != 1 || slice.ySampling != 1)
		{
			char *row_subsampled = (char *)slice.base + (slice.yStride * (dw.min.y + ((1 + dw.max.y - dw.min.y) / slice.ySampling) - 1)) + (slice.xStride * (dw.min.x + ((1 + dw.max.x - dw.min.x) / slice.ySampling) - 1));
			char *row_expanded = (char *)slice.base + (slice.yStride * dw.max.y) + (slice.xStride * dw.max.x);
			
			for(int y = dw.max.y; y >= dw.min.y; y--)
			{
				char *pix_subsampled = row_subsampled;
				char *pix_expanded = row_expanded;
				
				for(int x = dw.max.x; x >= dw.min.x; x--)
				{
					if(slice.type == Imf::FLOAT)
					{
						*((float *)pix_expanded) = *((float *)pix_subsampled);
					}
					else if(slice.type == Imf::UINT)
					{
						*((unsigned int *)pix_expanded) = *((unsigned int *)pix_subsampled);
					}
					
					if(x % slice.xSampling == 0)
						pix_subsampled -= slice.xStride;
					
					pix_expanded -= slice.xStride;
				}
				
				if(y % slice.ySampling == 0)
					row_subsampled -= slice.yStride;
				
				row_expanded -= slice.yStride;
			}
		}
	}
}


A_Err	
OpenEXR_DrawAuxChannel(
	AEIO_BasicData		*basic_dataP,
	OpenEXR_inData		*options,
	const A_PathType	*file_pathZ,
	A_long				chan_indexL,
	const AEIO_DrawFramePB	*pbP,  // but cancel/progress seems to break here
	PF_Point			scale,
	PF_ChannelChunk		*chunkP)
{
	A_Err	err		= A_Err_NONE,
			err2	= A_Err_NONE;


	// get the channel record
	PF_ChannelDesc desc = options->channel[chan_indexL];

	
	AEGP_SuiteHandler suites(basic_dataP->pica_basicP);

	PF_Handle temp_handle = NULL;
	
	try{
	
	// read the EXR
	IStreamPlatform instream(file_pathZ, basic_dataP->pica_basicP);
	
	if(gMemoryMap)
		instream.memoryMap();
	
	HybridInputFile in(instream);
	
	const Header &head = in.header(0);
	const ChannelList &channels = in.channels();
	
	const Box2i &dataW = in.dataWindow();
	const Box2i &dispW = in.displayWindow();
	
	assert(options->display_window == DW_DATA_WINDOW || options->display_window == DW_DISPLAY_WINDOW);
	
	const Box2i &sizeW = (options->display_window == DW_DISPLAY_WINDOW ? dispW : dataW);
	
	const int width = (sizeW.max.x - sizeW.min.x) + 1;
	const int height = (sizeW.max.y - sizeW.min.y) + 1;
	
	 
	bool require_dataW_buf = false;

	if(options->display_window == DW_DISPLAY_WINDOW)
	{
		// if the dataWindow does not actually intersect the displayWindow,
		// no need to continue further
		if( !dataW.intersects(dispW) )
		{
			return err;
		}
		
		// if any dataWindow pixels are outside the displayWindow, must create a temp buffer
		// once thought about optimizing for vertical-only case, but remembered we might get a channel cache unexpectedly
		if( (dataW.min.x < dispW.min.x) ||
			(dataW.min.y < dispW.min.y) ||
			(dataW.max.x > dispW.max.x) ||
			(dataW.max.y > dispW.max.y) )
		{
			require_dataW_buf = true;
		}
	}
	
	// If AE is at partial resolution, we use this to shift the pixel we sample instead of getting the upper right one
	// We're trying to get closer to AE's scaling algorithm, which we can only use on full ARGB buffers
	PF_Point shift;
	shift.h = (width % scale.h == 0) ? (scale.h - 1) : (width % scale.h - 1);
	shift.v = (height % scale.v == 0) ? (scale.v - 1) : (height % scale.v - 1);
	
	
	vector<string> layer_channels;
	
	// parse the channel name for multiple channels the long-form way
	ChannelEntry channel_entry(desc.name, desc.channel_type, chunkP->data_type);
	
	if(channel_entry.dimensions() == chunkP->dimensionL)
	{
		for(int i=0; i < channel_entry.dimensions(); i++)
			layer_channels.push_back(channel_entry.chan_part(i));
	}
	else
	{
		// must be short form, which is always something like layer.RGB
		// so we should be able to figure out the layer name from that
		
		string this_compound_layer_name = desc.name;
		
		string::size_type dot_pos = this_compound_layer_name.find_last_of('.');
		
		if(dot_pos != string::npos && dot_pos < (this_compound_layer_name.size() - 1))
		{
			string layer_name = this_compound_layer_name.substr(0, dot_pos);
			
			// duplicate code from above to figure out which channels make up this layer
			ChannelList::ConstIterator f, l;
			channels.channelsInLayer(layer_name, f, l);
				
			// put into a list so we can use the sort function
			list<string> channels_in_layer;
			
			for(ChannelList::ConstIterator j = f; j != l; j++)
				channels_in_layer.push_back(j.name());
			
			if(channels_in_layer.size() > 1)
			{
				// sort into RGBA
				channels_in_layer.sort(channelOrderCompare);
				
				// transfer from list to compound string, short form is assumed
				string compound_layer_name;
				vector<string> compound_layer_channels;
				
				for(list<string>::const_iterator j = channels_in_layer.begin(); j != channels_in_layer.end() && compound_layer_channels.size() < chunkP->dimensionL; j++)
				{
					if( compound_layer_name.empty() )
					{
						compound_layer_name = *j;
					}
					else
					{
						assert(dot_pos == j->find_last_of('.'));
						
						compound_layer_name += j->substr(dot_pos + 1);
					}
					
					compound_layer_channels.push_back(*j);
				}
				
				// is this the compound layer we're looking for?
				if(compound_layer_name == desc.name)
				{
					layer_channels = compound_layer_channels;
				}
			}
		}
	}
	

	if(layer_channels.size() == chunkP->dimensionL)
	{
		int channel_dims = chunkP->dimensionL;
		Imf::PixelType pix_type = (chunkP->data_type == PF_DataType_FLOAT ? Imf::FLOAT : Imf::UINT);
		size_t pix_size = (pix_type == Imf::FLOAT ? sizeof(float) : sizeof(unsigned int)); // yeah, probably the same either way, I know

		const int buf_width = require_dataW_buf ? (dataW.max.x - dataW.min.x) + 1 : width;
		const int buf_height = require_dataW_buf ? (dataW.max.y - dataW.min.y) + 1 : height;
		
		void *exr_buffer = NULL;
		size_t rowbytes = 0;
		
		// do we need to make a new buffer for the EXR to read into?
		void *temp_buffer = NULL;
		
		if(buf_width != chunkP->widthL || buf_height != chunkP->heightL || pix_type == Imf::UINT || require_dataW_buf)
		{
			rowbytes = pix_size * channel_dims * buf_width;
			
			temp_handle = suites.HandleSuite()->host_new_handle(rowbytes * buf_height);
			
			if(temp_handle == NULL)
				throw NullExc("Can't allocate a temp buffer like I need to.");
			
			exr_buffer = temp_buffer = suites.HandleSuite()->host_lock_handle(temp_handle);
		}
		else
		{
			rowbytes = chunkP->row_bytesL;
			
			exr_buffer = chunkP->dataPV;
			
			if(options->display_window == DW_DISPLAY_WINDOW)
			{
				// the dataWindow lives inside the displayWindow (maybe matches it exactly)
				exr_buffer = (char *)exr_buffer +
								(rowbytes * (dataW.min.y - dispW.min.y)) +
								((pix_size * channel_dims) * (dataW.min.x - dispW.min.x));
			}
		}
		

		// read channels from the EXR
		char *exr_origin = (char *)exr_buffer - (pix_size * channel_dims * dataW.min.x) - (rowbytes * dataW.min.y);
		
		FrameBuffer frameBuffer;
		
		for(int c=0; c < channel_dims; c++)
		{
			const string &channel_name = layer_channels[c];
			
			int xSampling = 1,
				ySampling = 1;
			
			const Channel *channel = channels.findChannel(channel_name);
			
			if(channel)
			{
				xSampling = channel->xSampling;
				ySampling = channel->ySampling;
			}
			
			frameBuffer.insert(channel_name,
								Slice(pix_type,
										exr_origin + (pix_size * c),
										pix_size * channel_dims,
										rowbytes,
										xSampling, 
										ySampling,
										0.0) );
		}


	#ifdef NDEBUG
		#define CONT2() ( (pbP && pbP->inter.abort0) ? !(err2 = pbP->inter.abort0(pbP->inter.refcon) ) : TRUE)
		#define PROG2(COUNT, TOTAL) ( (pbP && pbP->inter.progress0) ? !(err2 = pbP->inter.progress0(pbP->inter.refcon, COUNT, TOTAL) ) : TRUE)
		const AEIO_InterruptFuncs *interP = (pbP ? &pbP->inter : NULL);
	#else
		#define CONT2()					TRUE
		#define PROG2(COUNT, TOTAL)		TRUE
		const AEIO_InterruptFuncs *interP = NULL;
	#endif

		OpenEXR_ChannelCache *chan_cache = gCachePool.findCache(instream);
		
		if(chan_cache == NULL && options->cache_channels && gChannelCaches > 0 && CONT2())
		{
			try
			{
				chan_cache = gCachePool.addCache(in, instream, interP);
			}
			catch(CancelExc &e) { err2 = e.err(); }
		}
		
		
		if(!err && !err2)
		{
			if(chan_cache)
			{
				if( CONT2() )
				{
					chan_cache->fillFrameBuffer(frameBuffer, dataW);
				}
			}
			else
			{
				in.setFrameBuffer(frameBuffer);

				try
				{
					int begin_line = dataW.min.y;
					int end_line = dataW.max.y;
					
					if(options->display_window == DW_DISPLAY_WINDOW)
					{
						begin_line = max<int>(dataW.min.y, (dispW.min.y - (scale.v - 1))); // we read a little outside the required range when we're subsampled
						end_line = min<int>(dataW.max.y, (dispW.max.y + (scale.v - 1)));
					}
					
			
					const int scanline_block_size = ScanlineBlockSize(in);
					
					int y = begin_line;
					
					while(y <= end_line && PROG2(y - begin_line, end_line - begin_line) )
					{
						int high_scanline = min(y + scanline_block_size - 1, end_line);
						
						in.readPixels(y, high_scanline);
						
						y = high_scanline + 1;
					}
				}
				catch(IoExc) {} // we catch these so that partial files are read partially without error
				catch(InputExc) {}
				catch(...) { err = AEIO_Err_PARSING; }

				// deal with the rare occurrence of subsampling
				FixSubsampling(frameBuffer, dataW);
			}
		}
		
		// copy from a temp buffer to the AE one if necessary
		if(temp_buffer != NULL && !err && !err2)
		{
			PF_Point disp_origin, data_origin;
			int copy_width, copy_height;
			
			if(options->display_window == DW_DISPLAY_WINDOW)
			{
				if(dispW.min.x < dataW.min.x)
				{
					disp_origin.h = dataW.min.x - dispW.min.x;
					data_origin.h = 0;
				}
				else
				{
					disp_origin.h = 0;
					data_origin.h = dispW.min.x - dataW.min.x;
				}
				
				
				if(dispW.min.y < dataW.min.y)
				{
					disp_origin.v = dataW.min.y - dispW.min.y;
					data_origin.v = 0;
				}
				else
				{
					disp_origin.v = 0;
					data_origin.v = dispW.min.y - dataW.min.y;
				}
				
				copy_width = min(dispW.max.x, dataW.max.x) - max(dispW.min.x, dataW.min.x) + 1;
				copy_height = min(dispW.max.y, dataW.max.y) - max(dispW.min.y, dataW.min.y) + 1;
			}
			else
			{
				disp_origin.h = disp_origin.v = data_origin.h = data_origin.v = 0;
				
				copy_width = buf_width;
				copy_height = buf_height;
			}
			
			size_t chunk_pix_size = (	chunkP->data_type == PF_DataType_FLOAT			? 4 :
										chunkP->data_type == PF_DataType_DOUBLE			? 8 :
										chunkP->data_type == PF_DataType_LONG			? 4 :
										chunkP->data_type == PF_DataType_SHORT			? 2 :
										chunkP->data_type == PF_DataType_FIXED_16_16	? 4 :
										chunkP->data_type == PF_DataType_CHAR			? 1 :
										chunkP->data_type == PF_DataType_U_BYTE			? 1 :
										chunkP->data_type == PF_DataType_U_SHORT		? 2 :
										chunkP->data_type == PF_DataType_U_FIXED_16_16	? 4 :
										chunkP->data_type == PF_DataType_RGB			? 3 :
										4); // else?
									
			void *display_pixel_origin = (char *)chunkP->dataPV +
											((disp_origin.v / scale.v) * chunkP->row_bytesL) +
											((disp_origin.h / scale.h) * (chunk_pix_size * channel_dims));

			void *data_pixel_origin = (char *)temp_buffer +
										(data_origin.v * rowbytes) +
										(data_origin.h * (pix_size * channel_dims));
			
			
			copy_width = (copy_width / scale.h) + (copy_width % scale.h ? 1 : 0);
			copy_height = (copy_height / scale.v) + (copy_height % scale.v ? 1 : 0);
			
			assert(((disp_origin.v / scale.v) + copy_width) <= chunkP->widthL);
			assert(((disp_origin.h / scale.h) + copy_height) <= chunkP->heightL);
		

			IterateData i_data = { interP, data_pixel_origin, rowbytes, display_pixel_origin, chunkP->row_bytesL, channel_dims, copy_width, scale, shift };
		
			if(chunkP->data_type == PF_DataType_FLOAT)
				err2 = suites.AEGPIterateSuite()->AEGP_IterateGeneric(copy_height, (void *)&i_data, CopyBufferIterate<float, float>);
			else if(chunkP->data_type == PF_DataType_U_BYTE || chunkP->data_type == PF_DataType_CHAR)
				err2 = suites.AEGPIterateSuite()->AEGP_IterateGeneric(copy_height, (void *)&i_data, CopyBufferIterate<unsigned int, char>);
			else if(chunkP->data_type == PF_DataType_U_SHORT)
				err2 = suites.AEGPIterateSuite()->AEGP_IterateGeneric(copy_height, (void *)&i_data, CopyBufferIterate<unsigned int, unsigned short>);
			else if(chunkP->data_type == PF_DataType_SHORT)
				err2 = suites.AEGPIterateSuite()->AEGP_IterateGeneric(copy_height, (void *)&i_data, CopyBufferIterate<unsigned int, short>);
			else if(chunkP->data_type == PF_DataType_LONG)
				err2 = suites.AEGPIterateSuite()->AEGP_IterateGeneric(copy_height, (void *)&i_data, CopyBufferIterate<unsigned int, int>);
		}
	}
	
	} catch(...) { err = AEIO_Err_PARSING; }
	
	
	if(temp_handle)
		suites.HandleSuite()->host_dispose_handle(temp_handle);
		

	if(err2) { err = err2; }
	return err;
}



//	   input
// =====================================================================
//	   output

#pragma mark-


A_Err	
OpenEXR_GetDepths(
	AEIO_SupportedDepthFlags		*which)
{
	// these options will be avialable in the depth menu
	A_Err err						=	A_Err_NONE;

#define AEIO_SupportedDepthFlags_DEPTH_GRAY_32		(1L << 16)

	*which =	AEIO_SupportedDepthFlags_DEPTH_96	|
				AEIO_SupportedDepthFlags_DEPTH_128;
				
	// here's an easter egg to support Floating Point Gray
	// add a line like this to your channel map file:
	// Y	UNKN	FLT4
	// we have to do this because regular Floating Point Gray
	// gets selected by default in AE, which will confuse people
	if( gChannelMap->findEntry("Y") )
		*which |= AEIO_SupportedDepthFlags_DEPTH_GRAY_32;
				

	return err;
}


A_Err	
OpenEXR_InitOutOptions(
	OpenEXR_outData *options)
{
	// initialize the options when they're first created
	// will probably do this only once per AE user per version
	
	A_Err err						=	A_Err_NONE;
	
	options->compression_type = Imf::PIZ_COMPRESSION;
	options->float_not_half = FALSE;
	options->luminance_chroma = FALSE;

	return err;
}


typedef struct {
	void *in;
	size_t in_rowbytes;
	void *out;
	size_t out_rowbytes;
	int width;
} FloatToHalfData;

static A_Err
FloatToHalf_Iterate(
	void	*refconPV,
	A_long	thread_indexL,
	A_long	i,
	A_long	iterationsL)
{
	A_Err err = A_Err_NONE;
	
	FloatToHalfData *i_data = (FloatToHalfData *)refconPV;
	
	float *in = (float *)((char *)i_data->in + (i * i_data->in_rowbytes));
	half *out = (half *)((char *)i_data->out + (i * i_data->out_rowbytes));
	
	int width = 4 * i_data->width;
	
	while(width--)
	{
		*out++ = *in++;
	}
	
	return err;
}


A_Err
OpenEXR_OutputFile(
	AEIO_BasicData		*basic_dataP,
	const A_PathType	*file_pathZ,
	FrameSeq_Info		*info,
	OpenEXR_outData		*options,
	PF_EffectWorld		*wP)
{
	// write da file, mon

	A_Err err						=	A_Err_NONE;
	

	if(options == NULL) // we'll need this
		return AEIO_Err_INCONSISTENT_PARAMETERS;
	
	
	
	AEGP_SuiteHandler suites(basic_dataP->pica_basicP);
		
	AEGP_WorldH temp_worldH = NULL;
		

	try{
	
	int data_width, display_width, data_height, display_height;
	
	data_width = display_width = info->width;
	data_height = display_height = info->height;
	
	if(options->luminance_chroma)
	{	// Luminance/Chroma needs even width & height
		data_width += (data_width % 2);
		data_height += (data_height % 2);
	}
	
	
	// set up header
	Header header(	Box2i( V2i(0,0), V2i(display_width-1, display_height-1) ),
					Box2i( V2i(0,0), V2i(data_width-1, data_height-1) ),
					(float)info->pixel_aspect_ratio.num / (float)info->pixel_aspect_ratio.den,
					V2f(0, 0),
					1,
					INCREASING_Y,
					(Compression)options->compression_type);


	// store the actual ratio as a custom attribute
	if(info->pixel_aspect_ratio.num != info->pixel_aspect_ratio.den)
	{
		header.insert(PIXEL_ASPECT_RATIONAL_KEY,
						RationalAttribute( Rational(info->pixel_aspect_ratio.num, info->pixel_aspect_ratio.den) ) );
	}
	
	// store ICC Profile
	if(info->icc_profile && info->icc_profile_len)
	{
		addICCprofile(header, info->icc_profile, info->icc_profile_len);
	}
	
	// store chromaticities
	if(info->chromaticities)
	{
		// function from ImfStandardAttributes
		addChromaticities(header, Chromaticities(
									V2f(info->chromaticities->red.x, info->chromaticities->red.y),
									V2f(info->chromaticities->green.x, info->chromaticities->green.y),
									V2f(info->chromaticities->blue.x, info->chromaticities->blue.y),
									V2f(info->chromaticities->white.x, info->chromaticities->white.y) ) );

		// store the color space name in the file
		if( strlen(info->chromaticities->color_space_name) && strcmp(info->chromaticities->color_space_name, "OpenEXR") != 0 )
			header.insert(COLOR_SPACE_NAME_KEY, StringAttribute( string(info->chromaticities->color_space_name) ) );
	}
	
	
	// extra AE/platform attributes
	if(info->render_info)
	{
		if( strlen(info->render_info->proj_name) )
			header.insert("AfterEffectsProject", StringAttribute( string(info->render_info->proj_name) ) );

		if( strlen(info->render_info->comp_name) )
			header.insert("AfterEffectsComp", StringAttribute( string(info->render_info->comp_name) ) );

		if( strlen(info->render_info->comment_str) )
			addComments(header, string(info->render_info->comment_str) );

		if( gStoreMachine && strlen(info->render_info->computer_name) )
			header.insert("computerName", StringAttribute( string(info->render_info->computer_name) ) );

		if( gStorePersonal && strlen(info->render_info->user_name) )
			addOwner(header, string(info->render_info->user_name) );
		
		if( info->render_info->framerate.value )
			addFramesPerSecond(header, Rational(info->render_info->framerate.value, info->render_info->framerate.scale) );
	}
	
	
	// add time attributes
	time_t the_time = time(NULL);
	tm *local_time = localtime(&the_time);
	
	if(local_time)
	{
		char date_string[256];
		sprintf(date_string, "%04d:%02d:%02d %02d:%02d:%02d", local_time->tm_year + 1900, local_time->tm_mon + 1, local_time->tm_mday,
													local_time->tm_hour, local_time->tm_min, local_time->tm_sec);
		
		addCapDate(header, string(date_string) );
#ifdef WIN_ENV
		_timeb win_time;
		_ftime(&win_time);
		addUtcOffset(header, (float)( (win_time.timezone - (win_time.dstflag ? 60 : 0) ) * 60));
#else
		addUtcOffset(header, (float)-local_time->tm_gmtoff);
#endif
	}
	
	
	// graffiti
	header.insert("writer", StringAttribute( string("ProEXR for After Effects") ) );


	// write the file
	if(info->planes < 3 || options->luminance_chroma)
	{
		// we can use the RGBA (Half) output interface
		
		RgbaChannels rgba_channels =	(info->planes == 1) ? WRITE_Y :
										(info->planes == 2) ? WRITE_YA :
										(info->planes == 3) ? (options->luminance_chroma ? WRITE_YC : WRITE_RGB) :
										(info->planes == 4) ? (options->luminance_chroma ? WRITE_YCA : WRITE_RGBA) :
										WRITE_RGBA; // ummm?
		
		OStreamPlatform outstream(file_pathZ);
		RgbaOutputFile	outputFile(outstream, header, rgba_channels);
		

		// scanline buffer
		Array2D<Rgba> half_buffer(1, data_width);
		

		// AE buffer
		A_u_char *row = (A_u_char *)wP->data;
		
		PF_Pixel32 *pixel;
		
		
		for(int y=0; y < info->height; ++y)
		{
			pixel = (PF_Pixel32 *)row;
			
			for(int x=0; x < info->width; ++x)
			{
				half_buffer[0][x].r = pixel->red;
				half_buffer[0][x].g = pixel->green;
				half_buffer[0][x].b = pixel->blue;
				
				if(info->planes == 4)
					half_buffer[0][x].a = pixel->alpha;
				else
					half_buffer[0][x].a = half(1.0);
				
				pixel++;
			}
			
			if( data_width == (info->width + 1) ) // duplicate last pixel (for Luminance/Chroma)
			{	half_buffer[0][data_width - 1] = half_buffer[0][info->width - 1];	}
			
			// write scanline
			outputFile.setFrameBuffer(&half_buffer[-y][0], 1, data_width);
			outputFile.writePixels(1);
			
			row += wP->rowbytes;	
		}
		
		
		// write an extra duplicate line if necessary (for Luminance/Chroma)
		if( data_height == (info->height + 1) )
		{
			outputFile.setFrameBuffer(&half_buffer[1 - data_height][0], 1, data_width);
			outputFile.writePixels(1);
		}
	}
	else
	{
		Imf::PixelType pix_type = (options->float_not_half ? Imf::FLOAT : Imf::HALF);
		size_t pix_size = (pix_type == Imf::FLOAT ? sizeof(float) : sizeof(half));
		
		void *buf_origin = wP->data;
		size_t rowbytes = wP->rowbytes;
		
		
		// have to make a half buffer ourselves
		if(pix_type == Imf::HALF)
		{
			suites.AEGPWorldSuite()->AEGP_New(S_mem_id, AEGP_WorldType_16, data_width, data_height, &temp_worldH);
			
			A_u_long temp_rowbytes;
			suites.AEGPWorldSuite()->AEGP_GetRowBytes(temp_worldH, &temp_rowbytes);
			rowbytes = temp_rowbytes;
			
			PF_Pixel16 *base = NULL;
			suites.AEGPWorldSuite()->AEGP_GetBaseAddr16(temp_worldH, &base);
			buf_origin = (void *)base;
			
			FloatToHalfData i_data = { wP->data, wP->rowbytes, (void *)base, temp_rowbytes, data_width };
			
			err = suites.AEGPIterateSuite()->AEGP_IterateGeneric(data_height, (void *)&i_data, FloatToHalf_Iterate);
		}
		
		
		header.channels().insert("R", Channel(pix_type));
		header.channels().insert("G", Channel(pix_type));
		header.channels().insert("B", Channel(pix_type));
		
		if(info->planes == 4)
			header.channels().insert("A", Channel(pix_type));
		
		
		FrameBuffer frameBuffer;
		
		frameBuffer.insert("R", Slice(pix_type, (char *)buf_origin + (pix_size * 1), pix_size * 4, rowbytes) );
		frameBuffer.insert("G", Slice(pix_type, (char *)buf_origin + (pix_size * 2), pix_size * 4, rowbytes) );
		frameBuffer.insert("B", Slice(pix_type, (char *)buf_origin + (pix_size * 3), pix_size * 4, rowbytes) );
		
		if(info->planes == 4)
			frameBuffer.insert("A", Slice(pix_type, (char *)buf_origin + (pix_size * 0), pix_size * 4, rowbytes) );
		
		
		OStreamPlatform outstream(file_pathZ);
		OutputFile file(outstream, header);
		
		file.setFrameBuffer(frameBuffer);
		file.writePixels(data_height);
	}
	

	}catch(...) { err = AEIO_Err_DISK_FULL; }

		
	if(temp_worldH)
		suites.AEGPWorldSuite()->AEGP_Dispose(temp_worldH);
	
	return err;
}

A_Err	
OpenEXR_WriteOptionsDialog(
	AEIO_BasicData		*basic_dataP,
	OpenEXR_outData		*options,
	A_Boolean			*user_interactedPB0)
{
	if(options)
		OpenEXR_OutDialog(basic_dataP, options, user_interactedPB0);
	
	return A_Err_NONE;
}

A_Err	
OpenEXR_GetOutSpecInfo(
	const A_PathType	*file_pathZ,
	OpenEXR_outData		*options,
	AEIO_Verbiage		*verbiageP)
{ 
	// describe out output options state in English (or another language if you prefer)
	// only sub-type appears to work (but with carriage returs (\r) )
	A_Err err			=	A_Err_NONE;

	// actually, this shows up in the template
	strcpy(verbiageP->type, PLUGIN_NAME);

	switch(options->compression_type)
	{
		case Imf::NO_COMPRESSION:
			strcpy(verbiageP->sub_type, "No compression");
			break;

		case Imf::RLE_COMPRESSION:
			strcpy(verbiageP->sub_type, "RLE compression");
			break;

		case Imf::ZIPS_COMPRESSION:
			strcpy(verbiageP->sub_type, "Zip compression");
			break;

		case Imf::ZIP_COMPRESSION:
			strcpy(verbiageP->sub_type, "Zip compression\nin blocks of 16 scan lines");
			break;

		case Imf::PIZ_COMPRESSION:
			strcpy(verbiageP->sub_type, "Piz compression");
			break;

		case Imf::PXR24_COMPRESSION:
			strcpy(verbiageP->sub_type, "PXR24 compression");
			break;

		case Imf::B44_COMPRESSION:
			strcpy(verbiageP->sub_type, "B44 compression");
			break;
			
		case Imf::B44A_COMPRESSION:
			strcpy(verbiageP->sub_type, "B44A compression");
			break;

		default:
			strcpy(verbiageP->sub_type, "unknown compression!");
			break;
	}
	
	if(options->luminance_chroma)
		strcat(verbiageP->sub_type, "\nLuminance/Chroma");
	else if(options->float_not_half)
		strcat(verbiageP->sub_type, "\n32-bit float");
	
	return err;
}


A_Err
OpenEXR_FlattenOutputOptions(
	OpenEXR_outData *options)
{
	// no need to flatten or byte-flip or whatever
	
	return A_Err_NONE;
}

A_Err
OpenEXR_InflateOutputOptions(
	OpenEXR_outData *options)
{
	// just flip again
	return OpenEXR_FlattenOutputOptions(options);
}
