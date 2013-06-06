//
//	EXtractoR
//		by Brendan Bolles <brendan@fnordware.com>
//
//	extract float channels
//
//	Part of the fnord OpenEXR tools
//		http://www.fnordware.com/OpenEXR/
//
//

#include "EXtractoR.h"

#include "EXtractoR_Dialog.h"

#include <assert.h>

#include <float.h>

#include <string>
#include <list>

using namespace std;


static PF_Err 
About (	
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output )
{
	PF_SPRINTF(	out_data->return_msg, 
				"%s - %s\r\rwritten by %s\r\rv%d.%d - %s\r\r%s\r%s",
				NAME,
				DESCRIPTION,
				AUTHOR, 
				MAJOR_VERSION, 
				MINOR_VERSION,
				RELEASE_DATE,
				COPYRIGHT,
				WEBSITE);
				
	return PF_Err_NONE;
}


static PF_Err 
GlobalSetup (	
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output )
{
	//	We do very little here.
		
	out_data->my_version 	= 	PF_VERSION(	MAJOR_VERSION, 
											MINOR_VERSION,
											BUG_VERSION, 
											STAGE_VERSION, 
											BUILD_VERSION);

	out_data->out_flags 	= 	PF_OutFlag_DEEP_COLOR_AWARE		|
								PF_OutFlag_PIX_INDEPENDENT		|
								PF_OutFlag_CUSTOM_UI			|
							#ifdef WIN_ENV
								PF_OutFlag_KEEP_RESOURCE_OPEN	|
							#endif
								PF_OutFlag_USE_OUTPUT_EXTENT;

	out_data->out_flags2 	=	PF_OutFlag2_PARAM_GROUP_START_COLLAPSED_FLAG |
								PF_OutFlag2_SUPPORTS_SMART_RENDER	|
								PF_OutFlag2_FLOAT_COLOR_AWARE;
	
	return PF_Err_NONE;
}


static PF_Err
ParamsSetup(
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output)
{
	PF_Err 			err = PF_Err_NONE;
	PF_ParamDef		def;


	// readout
	AEFX_CLR_STRUCT(def);
	 // we can time_vary once we're willing to print and scan ArbData text
	def.flags = PF_ParamFlag_CANNOT_TIME_VARY;
	
	ArbNewDefault(in_data, out_data, ARB_REFCON, &def.u.arb_d.dephault);
	
	PF_ADD_ARBITRARY("Channel Info (Click for Dialog)",
						kUI_CONTROL_WIDTH,
						kUI_CONTROL_HEIGHT,
						PF_PUI_CONTROL,
						def.u.arb_d.dephault,
						ARBITRARY_DATA_ID,
						ARB_REFCON);
	
	
	AEFX_CLR_STRUCT(def);
	PF_ADD_TOPIC("Process", PROCESS_START_ID);

		AEFX_CLR_STRUCT(def);
		PF_ADD_FLOAT_SLIDER(	"Black Point",
						-FLT_MAX, FLT_MAX, -1000.0, 1000.0, // ranges
						0, // curve tolderance?
						0.0, // default
						2, 0, 0, // precision, display flags, want phase
						BLACK_ID);

		AEFX_CLR_STRUCT(def);
		PF_ADD_FLOAT_SLIDER(	"White Point",
						-FLT_MAX, FLT_MAX, -1000.0, 1000.0, // ranges
						0, // curve tolderance?
						1.0, // default
						2, 0, 0, // precision, display flags, want phase
						WHITE_ID);

		AEFX_CLR_STRUCT(def);
		PF_ADD_CHECKBOX("", "UnMult",
						FALSE, 0,
						UNMULT_ID);


	AEFX_CLR_STRUCT(def);
	PF_END_TOPIC(PROCESS_END_ID);

	out_data->num_params = EXTRACT_NUM_PARAMS;

	// register custom UI
	if (!err) 
	{
		PF_CustomUIInfo			ci;

		AEFX_CLR_STRUCT(ci);
		
		ci.events				= PF_CustomEFlag_EFFECT;
 		
		ci.comp_ui_width		= ci.comp_ui_height = 0;
		ci.comp_ui_alignment	= PF_UIAlignment_NONE;
		
		ci.layer_ui_width		= 0;
		ci.layer_ui_height		= 0;
		ci.layer_ui_alignment	= PF_UIAlignment_NONE;
		
		ci.preview_ui_width		= 0;
		ci.preview_ui_height	= 0;
		ci.layer_ui_alignment	= PF_UIAlignment_NONE;

		err = (*(in_data->inter.register_ui))(in_data->effect_ref, &ci);
	}


	return err;
}


static PF_Err
SequenceSetup (
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output )
{
	PF_Err err = PF_Err_NONE;
	
	ExtractStatus *extract_status = NULL;
	
	// set up sequence data
	if( (in_data->sequence_data == NULL) )
	{
		out_data->sequence_data = PF_NEW_HANDLE( sizeof(ExtractStatus) );
		
		extract_status = (ExtractStatus *)PF_LOCK_HANDLE(out_data->sequence_data);
	}
	else // reset pre-existing sequence data
	{
		if( PF_GET_HANDLE_SIZE(in_data->sequence_data) != sizeof(ExtractStatus) )
		{
			PF_RESIZE_HANDLE(sizeof(ExtractStatus), &in_data->sequence_data);
		}
			
		extract_status = (ExtractStatus *)PF_LOCK_HANDLE(in_data->sequence_data);
	}
	
	// set defaults
	extract_status->red.status = STATUS_UNKNOWN;
	extract_status->green.status = STATUS_UNKNOWN;
	extract_status->blue.status = STATUS_UNKNOWN;
	extract_status->alpha.status = STATUS_UNKNOWN;
	extract_status->compound.status = STATUS_UNKNOWN;
	
	PF_UNLOCK_HANDLE(in_data->sequence_data);
	
	return err;
}


static PF_Err 
SequenceSetdown (
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output )
{
	PF_Err err = PF_Err_NONE;
	
	if(in_data->sequence_data)
		PF_DISPOSE_HANDLE(in_data->sequence_data);

	return err;
}


static PF_Err 
SequenceFlatten (
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output )
{
	return PF_Err_NONE;
}


static PF_Boolean
IsEmptyRect(const PF_LRect *r){
	return (r->left >= r->right) || (r->top >= r->bottom);
}

#ifndef mmin
	#define mmin(a,b) ((a) < (b) ? (a) : (b))
	#define mmax(a,b) ((a) > (b) ? (a) : (b))
#endif


static void
UnionLRect(const PF_LRect *src, PF_LRect *dst)
{
	if (IsEmptyRect(dst)) {
		*dst = *src;
	} else if (!IsEmptyRect(src)) {
		dst->left 	= mmin(dst->left, src->left);
		dst->top  	= mmin(dst->top, src->top);
		dst->right 	= mmax(dst->right, src->right);
		dst->bottom = mmax(dst->bottom, src->bottom);
	}
}


static PF_Err
PreRender(
	PF_InData				*in_data,
	PF_OutData				*out_data,
	PF_PreRenderExtra		*extra)
{
	PF_Err err = PF_Err_NONE;
	PF_RenderRequest req = extra->input->output_request;
	PF_CheckoutResult in_result;
	
	req.preserve_rgb_of_zero_alpha = TRUE;

	ERR(extra->cb->checkout_layer(	in_data->effect_ref,
									EXTRACT_INPUT,
									EXTRACT_INPUT,
									&req,
									in_data->current_time,
									in_data->time_step,
									in_data->time_scale,
									&in_result));


	UnionLRect(&in_result.result_rect, 		&extra->output->result_rect);
	UnionLRect(&in_result.max_result_rect, 	&extra->output->max_result_rect);	
	
	// BTW, just because we checked out the layer here, doesn't mean we really
	// have to check it out.  
	
	return err;
}

#pragma mark-

static inline PF_FpShort clamp(PF_FpShort input)
{
	return (input > 1.0f ? 1.0f : (input < 0.0f ? 0.0f : input));
}

template <typename T>
static inline T
ConvertScale(PF_FpShort input, float black_point, float white_point, bool do_scale, bool do_clamp);

template <>
static inline PF_FpShort
ConvertScale<PF_FpShort>(PF_FpShort input, float black_point, float white_point, bool do_scale, bool do_clamp)
{
	float output = input;
	
	if(do_scale)
	{
		output = ( (input - black_point) / (white_point - black_point) );
	}
	
	if(do_clamp)
	{
		output = clamp(output);
	}
	
	return output;
}

template <>
static inline A_u_short
ConvertScale<A_u_short>(PF_FpShort input, float black_point, float white_point, bool do_scale, bool do_clamp)
{
	PF_FpShort scaled_input = ConvertScale<PF_FpShort>(input, black_point, white_point, do_scale, true);
	
	return ((scaled_input * (PF_FpShort)PF_MAX_CHAN16) + 0.5f);
}

template <>
static inline A_u_char
ConvertScale<A_u_char>(PF_FpShort input, float black_point, float white_point, bool do_scale, bool do_clamp)
{
	PF_FpShort scaled_input = ConvertScale<PF_FpShort>(input, black_point, white_point, do_scale, true);
	
	return ((scaled_input * (PF_FpShort)PF_MAX_CHAN8) + 0.5f);
}


typedef struct {
	PF_InData *in_data;
	void *in_buffer;
	int in_stride;
	size_t in_rowbytes;
	float black_point;
	float white_point;
	bool do_scale;
	bool do_clamp;
	void *out_buffer;
	int out_stride;
	size_t out_rowbytes;
	int width;
} IterateData;

template <typename T>
static PF_Err
CopyChannel_FloatScaleIterate(void *refconPV,
								A_long thread_indexL,
								A_long i,
								A_long iterationsL)
{
	PF_Err err = PF_Err_NONE;
	
	IterateData *i_data = (IterateData *)refconPV;
	PF_InData *in_data = i_data->in_data;
	
	float *in_pix = (float *)((char *)i_data->in_buffer + (i * i_data->in_rowbytes)); 
	T *out_pix = (T *)((char *)i_data->out_buffer + (i * i_data->out_rowbytes));
	
	bool do_scale = (i_data->do_scale && (i_data->black_point != 0.0f || i_data->white_point != 1.0f));
	
	for(int x=0; x < i_data->width; x++)
	{
		*out_pix = ConvertScale<T>(*in_pix, i_data->black_point, i_data->white_point, do_scale, i_data->do_clamp);
		
		in_pix += i_data->in_stride;
		out_pix += i_data->out_stride;
	}
	
#ifdef NDEBUG
	if(thread_indexL == 0)
		err = PF_ABORT(in_data);
#endif

	return err;
}

static PF_Err
CopyChannel_FloatToAE(PF_InData *in_data,
						float *input, int in_stride, size_t in_rowbytes, float black_point, float white_point, bool is_alpha,
						PF_PixelPtr out_pixels, PF_PixelFormat out_format, int out_channel, size_t out_rowbytes,
						int width, int height)
{
	PF_Err err = PF_Err_NONE;

	AEGP_SuiteHandler suites(in_data->pica_basicP);	
	
	IterateData i_data = { in_data, (void *)input, in_stride, in_rowbytes, black_point, white_point, !is_alpha, is_alpha, NULL, 4, out_rowbytes, width };
	
	if(out_format == PF_PixelFormat_ARGB128)
	{
		PF_FpShort *channel = (PF_FpShort *)out_pixels + out_channel;
		
		i_data.out_buffer = (void *)channel;
		
		err = suites.PFIterate8Suite()->iterate_generic(height, &i_data, CopyChannel_FloatScaleIterate<PF_FpShort>);
	}
	else if(out_format == PF_PixelFormat_ARGB64)
	{
		A_u_short *channel = (A_u_short *)out_pixels + out_channel;
		
		i_data.out_buffer = (void *)channel;
		
		err = suites.PFIterate8Suite()->iterate_generic(height, &i_data, CopyChannel_FloatScaleIterate<A_u_short>);
	}
	else if(out_format == PF_PixelFormat_ARGB32)
	{
		A_u_char *channel = (A_u_char *)out_pixels + out_channel;
		
		i_data.out_buffer = (void *)channel;
		
		err = suites.PFIterate8Suite()->iterate_generic(height, &i_data, CopyChannel_FloatScaleIterate<A_u_char>);
	}
	
	return err;
}


template <typename T>
static PF_Err
CopyChannel_Iterate(void *refconPV,
					A_long thread_indexL,
					A_long i,
					A_long iterationsL)
{
	PF_Err err = PF_Err_NONE;
	
	IterateData *i_data = (IterateData *)refconPV;
	PF_InData *in_data = i_data->in_data;
	
	T *in_pix = (T *)((char *)i_data->in_buffer + (i * i_data->in_rowbytes)); 
	T *out_pix = (T *)((char *)i_data->out_buffer + (i * i_data->out_rowbytes));
	
	for(int x=0; x < i_data->width; x++)
	{
		*out_pix = *in_pix;
		
		in_pix += i_data->in_stride;
		out_pix += i_data->out_stride;
	}
	
#ifdef NDEBUG
	if(thread_indexL == 0)
		err = PF_ABORT(in_data);
#endif

	return err;
}


static PF_Err
CopyChannel_AEtoAE(PF_InData *in_data,
					PF_PixelFormat format,
					PF_EffectWorld	*input,
					PF_EffectWorld	*output,
					int channel)
{
	PF_Err err = PF_Err_NONE;
	
	assert(input != NULL);
	
	if(input == NULL)
		return PF_Err_BAD_CALLBACK_PARAM;

	AEGP_SuiteHandler suites(in_data->pica_basicP);
	
	IterateData i_data = { in_data, NULL, 4, input->rowbytes, 0.0f, 1.0f, false, false, NULL, 4, output->rowbytes, output->width };
	
	if(format == PF_PixelFormat_ARGB128)
	{
		PF_FpShort *in = (PF_FpShort *)input->data + channel;
		PF_FpShort *out = (PF_FpShort *)output->data + channel;
		
		i_data.in_buffer = (void *)in;
		i_data.out_buffer = (void *)out;
		
		err = suites.PFIterate8Suite()->iterate_generic(output->height, &i_data, CopyChannel_Iterate<PF_FpShort>);
	}
	else if(format == PF_PixelFormat_ARGB64)
	{
		A_u_short *in = (A_u_short *)input->data + channel;
		A_u_short *out = (A_u_short *)output->data + channel;
		
		i_data.in_buffer = (void *)in;
		i_data.out_buffer = (void *)out;
		
		err = suites.PFIterate8Suite()->iterate_generic(output->height, &i_data, CopyChannel_Iterate<A_u_short>);
	}
	else if(format == PF_PixelFormat_ARGB32)
	{
		A_u_char *in = (A_u_char *)input->data + channel;
		A_u_char *out = (A_u_char *)output->data + channel;
		
		i_data.in_buffer = (void *)in;
		i_data.out_buffer = (void *)out;
		
		err = suites.PFIterate8Suite()->iterate_generic(output->height, &i_data, CopyChannel_Iterate<A_u_char>);
	}
	
	return err;
}


template <typename T>
static inline T
GetFill(float f_fill);

template <>
static inline PF_FpShort
GetFill<PF_FpShort>(float f_fill)
{
	return f_fill;
}

template <>
static inline A_u_short
GetFill<A_u_short>(float f_fill)
{
	return (f_fill * (float)PF_MAX_CHAN16) + 0.5f;
}

template <>
static inline A_u_char
GetFill<A_u_char>(float f_fill)
{
	return (f_fill * (float)PF_MAX_CHAN8) + 0.5f;
}

template <typename T>
static PF_Err
FillChannel_Iterate(void *refconPV,
					A_long thread_indexL,
					A_long i,
					A_long iterationsL)
{
	PF_Err err = PF_Err_NONE;
	
	IterateData *i_data = (IterateData *)refconPV;
	PF_InData *in_data = i_data->in_data;
	
	T *out_pix = (T *)((char *)i_data->out_buffer + (i * i_data->out_rowbytes));
	
	T fill_value = GetFill<T>(i_data->black_point);
	
	for(int x=0; x < i_data->width; x++)
	{
		*out_pix = fill_value;
		
		out_pix += i_data->out_stride;
	}
	
#ifdef NDEBUG
	if(thread_indexL == 0)
		err = PF_ABORT(in_data);
#endif
	
	return err;
}

static PF_Err
FillChannel_AE(PF_InData *in_data,
				PF_PixelFormat format,
				PF_EffectWorld	*world,
				int channel,
				A_Boolean fill_white)
{
	PF_Err err = PF_Err_NONE;

	AEGP_SuiteHandler suites(in_data->pica_basicP);
	
	IterateData i_data = { in_data, NULL, 0, 0, 0.0f, 1.0f, false, false, NULL, 4, world->rowbytes, world->width };
	
	i_data.black_point = (fill_white ? 1.0f : 0.0f);
	
	if(format == PF_PixelFormat_ARGB128)
	{
		PF_FpShort *buf = (PF_FpShort *)world->data + channel;
		
		i_data.out_buffer = (void *)buf;
		
		err = suites.PFIterate8Suite()->iterate_generic(world->height, &i_data, FillChannel_Iterate<PF_FpShort>);
	}
	else if(format == PF_PixelFormat_ARGB64)
	{
		A_u_short *buf = (A_u_short *)world->data + channel;
		
		i_data.out_buffer = (void *)buf;
		
		err = suites.PFIterate8Suite()->iterate_generic(world->height, &i_data, FillChannel_Iterate<A_u_short>);
	}
	else if(format == PF_PixelFormat_ARGB32)
	{
		A_u_char *buf = (A_u_char *)world->data + channel;
		
		i_data.out_buffer = (void *)buf;
		
		err = suites.PFIterate8Suite()->iterate_generic(world->height, &i_data, FillChannel_Iterate<A_u_char>);
	}
	
	return err;
}

#pragma mark-

static PF_Err
DoRender(
		PF_InData		*in_data,
		PF_EffectWorld 	*input,
		PF_ParamDef		*EXTRACT_data,
		PF_ParamDef		*EXTRACT_black,
		PF_ParamDef		*EXTRACT_white,
		PF_ParamDef		*EXTRACT_unmult,
		PF_OutData		*out_data,
		PF_EffectWorld	*output)
{
	PF_Err				err 	= PF_Err_NONE,
						err2 	= PF_Err_NONE,
						err3	= PF_Err_NONE;
	
	AEGP_SuiteHandler suites(in_data->pica_basicP);

	PF_PixelFormat		format	=	PF_PixelFormat_INVALID;
	PF_WorldSuite		*wsP	=	suites.PFWorldSuite();
	PF_FillMatteSuite	*fm		=	suites.PFFillMatteSuite();
	PF_ChannelSuite		*cs		=	suites.PFChannelSuite();

	if(!err)
	{

		err = wsP->PF_GetPixelFormat(output, &format);
		
		// some sizes we'll need
		size_t sizeof_pixel, sizeof_channel;
		size_t sizeof_chunkpix = sizeof(float); // always float in EXtractoR

		if(format == PF_PixelFormat_ARGB32)
		{
			sizeof_pixel = sizeof(PF_Pixel);
			sizeof_channel = sizeof(A_u_char);
		}
		else if(format == PF_PixelFormat_ARGB64)
		{
			sizeof_pixel = sizeof(PF_Pixel16);
			sizeof_channel = sizeof(A_u_short);
		}
		if(format == PF_PixelFormat_ARGB128)
		{
			sizeof_pixel = sizeof(PF_Pixel32);
			sizeof_channel = sizeof(PF_FpShort);
		}


		// start with a copy
		//PF_COPY(input, output, &src_rect, NULL);
	
		ArbitraryData *arb_data = (ArbitraryData *)PF_LOCK_HANDLE(EXTRACT_data->u.arb_d.value);
		ExtractStatus *extract_status = (ExtractStatus *)PF_LOCK_HANDLE(in_data->sequence_data);
		
		ChannelData compound_channel_data;
		
	#define EXTRACT_CHANNELS	4
	#define SEARCH_CHANNELS		5

		ChannelData		*channel_data[SEARCH_CHANNELS] = {	&arb_data->alpha,
															&arb_data->red,
															&arb_data->green,
															&arb_data->blue,
															&compound_channel_data };
															
		ChannelStatus	*channel_status[SEARCH_CHANNELS] = {	&extract_status->alpha,
																&extract_status->red,
																&extract_status->green,
																&extract_status->blue,
																&extract_status->compound };
								
		PF_Boolean		channel_found[SEARCH_CHANNELS] = { FALSE, FALSE, FALSE, FALSE, FALSE };
		
		PF_ChannelRef	chan_ref[SEARCH_CHANNELS];
		PF_ChannelDesc	chan_desc[SEARCH_CHANNELS];
		
		
		// set up the compound channel
	#define COMPOUND_INDEX		4
		compound_channel_data.action = DO_EXTRACT;
		compound_channel_data.index = extract_status->compound.index;
		compound_channel_data.name[0] = '\0';
		
		int compound_dimensions = 0;
		int compund_channel_locations[EXTRACT_CHANNELS] = {-1, -1, -1, -1};
		
		// this code should somewhat mirror the short form code in OpenEXR.cpp
		std::list<std::string> channels_in_layer;
		
		for(int i=0; i < EXTRACT_CHANNELS; i++)
		{
			if(channel_data[i]->action == DO_EXTRACT)
			{
				channels_in_layer.push_back(channel_data[i]->name);
				
				compund_channel_locations[i] = compound_dimensions;
				
				compound_dimensions++;
			}
			else
				compund_channel_locations[i] = -1;
		}
		
		assert(compound_dimensions == channels_in_layer.size());
		
		if(compound_dimensions > 1)
		{
			bool short_form = true;
			
			for(std::list<std::string>::const_iterator j = channels_in_layer.begin(); j != channels_in_layer.end() && short_form; j++)
			{
				const std::string &ref_channel = channels_in_layer.front();
				
				if(	j->find_last_of('.') == std::string::npos ||
					j->find_last_of('.') == (j->size() - 1) ||
					j->find_last_of('.') != ref_channel.find_last_of('.') ||
					j->substr(0, j->find_last_of('.')) != ref_channel.substr(0, ref_channel.find_last_of('.')) )
				{
					short_form = false;
				}
			}
			
			std::string compound_layer_name;
			
			for(std::list<std::string>::const_iterator j = channels_in_layer.begin(); j != channels_in_layer.end(); j++)
			{
				if( compound_layer_name.empty() )
				{
					compound_layer_name = *j;
				}
				else
				{
					if(short_form)
					{
						compound_layer_name += j->substr(j->find_last_of('.') + 1);
					}
					else
					{
						compound_layer_name += "|" + *j;
					}
				}
			}
			
			if(compound_layer_name.size() <= MAX_CHANNEL_NAME_LEN)
			{
				strcpy(compound_channel_data.name, compound_layer_name.c_str());
			}
			else
			{
				compound_channel_data.action = DO_NOTHING;
			}
		}
		else
		{
			compound_channel_data.action = DO_NOTHING;
		}
		
		
		// search for channels that match our channel names
		A_long num_channels;
		cs->PF_GetLayerChannelCount(in_data->effect_ref, EXTRACT_INPUT, &num_channels);
		
		for(int i=0; i < SEARCH_CHANNELS && !err && !err2 && !err3; i++)
		{
			if(channel_data[i]->action == DO_EXTRACT)
			{
				if(num_channels > 0) // don't bother if there's no channels
				{
					// remember index result between renders to prevent searching
					int channel_index = channel_data[i]->index;
					
					if(channel_status[i]->status == STATUS_INDEX_CHANGE &&
						channel_data[i]->index != channel_status[i]->index)
					{
						channel_index = channel_status[i]->index;
					}
					else
						channel_status[i]->status = STATUS_NORMAL;
			
					
					// get the channel
					cs->PF_GetLayerChannelIndexedRefAndDesc(in_data->effect_ref,
															EXTRACT_INPUT,
															MAX(0, MIN(channel_index, num_channels-1)),
															&channel_found[i],
															&chan_ref[i],
															&chan_desc[i]);
				}
				else
					channel_status[i]->status = STATUS_NO_CHANNELS; // that's right, nothing to find
				
				
				// found isn't really what it used to be
				channel_found[i] = (channel_found[i] && chan_desc[i].channel_type && chan_desc[i].data_type);
				
				// does the channel name match what we stored?
				if(channel_found[i] && 0 != strcmp(channel_data[i]->name, chan_desc[i].name) )
				{
					// uh oh, let's go searching for one that does					
					PF_Boolean found_replacement = FALSE;
					
					for(int j=0; j < num_channels && !found_replacement; j++)
					{
						// get the channel
						cs->PF_GetLayerChannelIndexedRefAndDesc(in_data->effect_ref,
																EXTRACT_INPUT,
																j,
																&channel_found[i],
																&chan_ref[i],
																&chan_desc[i]);
																
						// found isn't really what it used to be
						channel_found[i] = (channel_found[i] && chan_desc[i].channel_type && chan_desc[i].data_type);
						
						if(channel_found[i] && !strcmp(channel_data[i]->name, chan_desc[i].name) )
						{
							channel_data[i]->index = j; // this won't actually get set in the parameters
							found_replacement = TRUE;
							//EXTRACT_data->uu.change_flags = PF_ChangeFlag_CHANGED_VALUE;
							
							// report back on the new ID
							channel_status[i]->status = STATUS_INDEX_CHANGE;
							channel_status[i]->index = channel_data[i]->index;
						}
					}

					if(!found_replacement)
					{
						// never found the original channel name
						channel_status[i]->status = STATUS_NOT_FOUND;
						channel_found[i] = FALSE;
						
						// force a UI refresh after status has been updated
						out_data->out_flags |= PF_OutFlag_REFRESH_UI;
					}
				}
			}
		}
		
				
		// do compound channel if possible, otherwise one at a time
		if(channel_found[COMPOUND_INDEX] && chan_desc[COMPOUND_INDEX].data_type == PF_DataType_FLOAT && chan_desc[COMPOUND_INDEX].dimension == compound_dimensions)
		{
			PF_ChannelChunk chan_chunk;
		
			err = cs->PF_CheckoutLayerChannel(in_data->effect_ref,
											&chan_ref[COMPOUND_INDEX],
											in_data->current_time,
											in_data->time_step,
											in_data->time_scale,
											chan_desc[COMPOUND_INDEX].data_type,
											&chan_chunk);
											
			if(!err && !chan_chunk.dataPV)
				err = PF_Err_INVALID_CALLBACK; // there was a problem, dude
			
			if(!err)
			{
				sizeof_chunkpix = sizeof(float) * chan_chunk.dimensionL;
				
			#ifdef NDEBUG
				err2 = PF_ABORT(in_data);
			#endif
				
				for(int i=0; i < EXTRACT_CHANNELS && !err && !err2 && !err3; i++)
				{
					if(compund_channel_locations[i] >= 0)
					{
						char *world_row = (char *)output->data;
						char *chan_row = (char *)chan_chunk.dataPV;

						
						// the origin might not be 0,0 and the ROI might not include the whole image
						// we have to figure out where we have to move our pointers to the right spot in each buffer
						// and copy only as far as we can

						// if the origin is negative, we move in the world, if positive, we move in the channel
						PF_Point world_move, chan_move;
						
						world_move.h = MAX(-output->origin_x, 0);
						world_move.v = MAX(-output->origin_y, 0);
						
						chan_move.h = MAX(output->origin_x, 0);
						chan_move.v = MAX(output->origin_y, 0);

						int copy_height = MIN(output->height - world_move.v, chan_chunk.heightL - chan_move.v);
						int copy_width = MIN(output->width - world_move.h, chan_chunk.widthL - chan_move.h);

						// move pointers to starting pixel positions
						world_row += (world_move.v * output->rowbytes) + (world_move.h * sizeof_pixel);
						chan_row += (chan_move.v * chan_chunk.row_bytesL) + (chan_move.h * sizeof_chunkpix);
						
						
						chan_row += (compund_channel_locations[i] * sizeof(float));


						err2 = CopyChannel_FloatToAE(in_data,
													(PF_FpShort *)chan_row, chan_chunk.dimensionL, chan_chunk.row_bytesL,
													EXTRACT_black->u.fs_d.value, EXTRACT_white->u.fs_d.value, (i == 0),
													(PF_PixelPtr)world_row, format, i, output->rowbytes,
													copy_width, copy_height);
					}
					else
					{
						// this channel is being copied
						err2 = CopyChannel_AEtoAE(in_data, format, input, output, i);
					}
				}
				
				// check that channel back in
				err = cs->PF_CheckinLayerChannel(	in_data->effect_ref,
													&chan_ref[COMPOUND_INDEX],
													&chan_chunk);
			}
		}
		else
		{
		#ifdef NDEBUG
			#define PROG(CURR, TOTAL)	!(err3 = PF_PROGRESS(in_data, CURR, TOTAL))
		#else
			#define PROG(CURR, TOTAL)	TRUE
		#endif
		
			for(int i=0; i < EXTRACT_CHANNELS && !err && !err2 && !err3 && PROG(i, EXTRACT_CHANNELS); i++)
			{
				if(channel_data[i]->action == DO_EXTRACT)
				{
					//get the channel
					if(channel_found[i] && chan_desc[i].data_type == PF_DataType_FLOAT && chan_desc[i].dimension == 1) // we only handle one channel at a time here
					{
						PF_ChannelChunk chan_chunk;
					
						err = cs->PF_CheckoutLayerChannel(in_data->effect_ref,
														&chan_ref[i],
														in_data->current_time,
														in_data->time_step,
														in_data->time_scale,
														chan_desc[i].data_type,
														&chan_chunk);
														
						if(!err && !chan_chunk.dataPV)
							err = PF_Err_INVALID_CALLBACK; // there was a problem, dude
						
						// copy float values to the output buffer
						if(!err)
						{	
							char *world_row = (char *)output->data;
							char *chan_row = (char *)chan_chunk.dataPV;

							
							// the origin might not be 0,0 and the ROI might not include the whole image
							// we have to figure out where we have to move our pointers to the right spot in each buffer
							// and copy only as far as we can

							// if the origin is negative, we move in the world, if positive, we move in the channel
							PF_Point world_move, chan_move;
							
							world_move.h = MAX(-output->origin_x, 0);
							world_move.v = MAX(-output->origin_y, 0);
							
							chan_move.h = MAX(output->origin_x, 0);
							chan_move.v = MAX(output->origin_y, 0);

							int copy_height = MIN(output->height - world_move.v, chan_chunk.heightL - chan_move.v);
							int copy_width = MIN(output->width - world_move.h, chan_chunk.widthL - chan_move.h);

							// move pointers to starting pixel positions
							world_row += (world_move.v * output->rowbytes) + (world_move.h * sizeof_pixel);
							chan_row += (chan_move.v * chan_chunk.row_bytesL) + (chan_move.h * sizeof_chunkpix);
							

							err2 = CopyChannel_FloatToAE(in_data,
														(PF_FpShort *)chan_row, chan_chunk.dimensionL, chan_chunk.row_bytesL,
														EXTRACT_black->u.fs_d.value, EXTRACT_white->u.fs_d.value, (i == 0),
														(PF_PixelPtr)world_row, format, i, output->rowbytes,
														copy_width, copy_height);
													
													
							// check that channel back in
							err = cs->PF_CheckinLayerChannel(	in_data->effect_ref,
																&chan_ref[i],
																&chan_chunk);
						}
					}
					
					// if not found or something, fill black (white for alpha)
					if(!channel_found[i] || err)
					{
						if(channel_found[i])
						{
							channel_status[i]->status = STATUS_ERROR; // channel was found, but rejected
		
							// force a UI refresh after status has been updated
							out_data->out_flags |= PF_OutFlag_REFRESH_UI;
						}

						err2 = FillChannel_AE(in_data, format, output, i, (i == 0));
					}
				}
				else if(channel_data[i]->action == DO_COPY)
				{
					// not gonna extract, just copy the channel
					err2 = CopyChannel_AEtoAE(in_data, format, input, output, i);
				}
			}
		}
		
		// UnMult
		if(EXTRACT_unmult->u.bd.value)
			fm->premultiply(in_data->effect_ref, FALSE, output);
			
			
		PF_UNLOCK_HANDLE(EXTRACT_data->u.arb_d.value);
		PF_UNLOCK_HANDLE(in_data->sequence_data);
	}


	// error pecking order
	if(err3)
		err = err3;
	else if(err2)
		err = err2;
		

	return err;
}


static PF_Err
SmartRender(
	PF_InData				*in_data,
	PF_OutData				*out_data,
	PF_SmartRenderExtra		*extra)

{
	PF_Err			err		= PF_Err_NONE,
					err2 	= PF_Err_NONE;
					
	PF_EffectWorld *input, *output;
	
	PF_ParamDef EXTRACT_data,
				EXTRACT_black,
				EXTRACT_white,
				EXTRACT_unmult;

	// zero-out parameters
	AEFX_CLR_STRUCT(EXTRACT_data);
	AEFX_CLR_STRUCT(EXTRACT_black);
	AEFX_CLR_STRUCT(EXTRACT_white);
	AEFX_CLR_STRUCT(EXTRACT_unmult);
	
	
#define PF_CHECKOUT_PARAM_NOW( PARAM, DEST )	PF_CHECKOUT_PARAM(	in_data, (PARAM), in_data->current_time, in_data->time_step, in_data->time_scale, DEST )

	// get our arb data and see if it requires the input buffer
	err = PF_CHECKOUT_PARAM_NOW(EXTRACT_DATA, &EXTRACT_data);
	
	if(!err)
	{
		ArbitraryData *arb_data = (ArbitraryData *)PF_LOCK_HANDLE(EXTRACT_data.u.arb_d.value);

		if(arb_data->alpha.action == DO_COPY ||
			arb_data->red.action == DO_COPY ||
			arb_data->green.action == DO_COPY ||
			arb_data->blue.action == DO_COPY)
		{
			err = extra->cb->checkout_layer_pixels(in_data->effect_ref, EXTRACT_INPUT, &input);
		}
		else
		{
			input = NULL;
		}
		
		PF_UNLOCK_HANDLE(EXTRACT_data.u.arb_d.value);
	}
	
	
	// always get the output buffer
	ERR(	extra->cb->checkout_output(	in_data->effect_ref, &output)	);


	// checkout the required params
	ERR(	PF_CHECKOUT_PARAM_NOW( EXTRACT_BLACK,	&EXTRACT_black )	);
	ERR(	PF_CHECKOUT_PARAM_NOW( EXTRACT_WHITE,	&EXTRACT_white )	);
	ERR(	PF_CHECKOUT_PARAM_NOW( EXTRACT_UNMULT,	&EXTRACT_unmult )	);

	ERR(DoRender(	in_data, 
					input, 
					&EXTRACT_data,
					&EXTRACT_black,
					&EXTRACT_white,
					&EXTRACT_unmult,
					out_data, 
					output));

	// Always check in, no matter what the error condition!
	ERR2(	PF_CHECKIN_PARAM(in_data, &EXTRACT_data )	);
	ERR2(	PF_CHECKIN_PARAM(in_data, &EXTRACT_black )	);
	ERR2(	PF_CHECKIN_PARAM(in_data, &EXTRACT_white )	);
	ERR2(	PF_CHECKIN_PARAM(in_data, &EXTRACT_unmult )	);


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
	static const char * const channel_list[] = {"R", "r", "red",
												"G", "g", "green",
												"B", "b", "blue",
												"A", "a", "alpha"};
							
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


PF_Err 
DoDialog(
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output )
{
	A_Err			err 		= A_Err_NONE;
	
	AEGP_SuiteHandler suites(in_data->pica_basicP);
	PF_ChannelSuite *cs = suites.PFChannelSuite();


	A_long chan_count = 0;
	cs->PF_GetLayerChannelCount(in_data->effect_ref, 0, &chan_count);

	if(chan_count == 0 || err)
	{
		PF_SPRINTF(out_data->return_msg, "No auxiliary channels available.");
	}
	else
	{
		ArbitraryData *arb_data = (ArbitraryData *)PF_LOCK_HANDLE(params[EXTRACT_DATA]->u.arb_d.value);
		ExtractStatus *extract_status = (ExtractStatus *)PF_LOCK_HANDLE(in_data->sequence_data);
		
		// reconcile incorrect indices
		if(extract_status->red.status == STATUS_INDEX_CHANGE) { arb_data->red.index = extract_status->red.index; }
		if(extract_status->green.status == STATUS_INDEX_CHANGE) { arb_data->green.index = extract_status->green.index; }
		if(extract_status->blue.status == STATUS_INDEX_CHANGE) { arb_data->blue.index = extract_status->blue.index; }
		if(extract_status->alpha.status == STATUS_INDEX_CHANGE) { arb_data->alpha.index = extract_status->alpha.index; }
		
		
		Imf::ChannelList channels;
		map<string, int> index_map;
		
		A_long chan_count;
		cs->PF_GetLayerChannelCount(in_data->effect_ref, 0, &chan_count);
		
		for(int i=0; i < chan_count; i++)
		{
			PF_Boolean	found;

			PF_ChannelRef chan_ref;
			PF_ChannelDesc chan_desc;
			
			// get the channel
			cs->PF_GetLayerChannelIndexedRefAndDesc(in_data->effect_ref,
													0,
													i,
													&found,
													&chan_ref,
													&chan_desc);
													
			// found isn't really what it used to be
			found = (found && chan_desc.channel_type && chan_desc.data_type);
			
			if(found && chan_desc.dimension == 1 && chan_desc.data_type == PF_DataType_FLOAT)
			{
				channels.insert(chan_desc.name, Imf::Channel(Imf::FLOAT));
				index_map[chan_desc.name] = i;
			}
		}
		
		
		if(channels.begin() != channels.end()) // i.e. not empty
		{
			// use EXR code to merge channels into layers
			set<string> layerNames;
			channels.layers(layerNames);
			
			LayerMap layers;
			
			for(set<string>::const_iterator i = layerNames.begin(); i != layerNames.end(); i++)
			{
				// put into a list so we can use the sort function
				Imf::ChannelList::ConstIterator f, l;
				channels.channelsInLayer(*i, f, l);
				
				list<string> channels_in_layer;
				
				for(Imf::ChannelList::ConstIterator j = f; j != l; j++)
					channels_in_layer.push_back(j.name());
				
				if(channels_in_layer.size() > 1)
				{
					// sort into RGBA
					channels_in_layer.sort(channelOrderCompare);
				}
				
				// copy our list into a vector
				ChannelVec chans;
				
				for(list<string>::const_iterator j = channels_in_layer.begin(); j != channels_in_layer.end() && chans.size() < 4; ++j)
				{
					chans.push_back(*j);
				}
				
				layers[*i] = chans;
			}
			
			ChannelData	*channel_data[4] = {&arb_data->red,
											&arb_data->green,
											&arb_data->blue,
											&arb_data->alpha };
			string chan_names[4];
			
			for(int i=0; i < 4; i++)
			{
				if(channel_data[i]->action == DO_COPY)
				{
					chan_names[i] = "(copy)";
				}
				else
					chan_names[i] = channel_data[i]->name;
			}
			
			string &red = chan_names[0];
			string &green = chan_names[1];
			string &blue = chan_names[2];
			string &alpha = chan_names[3];
			

		#ifdef MAC_ENV
			const char *plugHndl = "com.adobe.AfterEffects.EXtractoR";
			const void *mwnd = NULL;
		#else
			const char *plugHndl = NULL;
			HWND *mwnd = NULL;
			PF_GET_PLATFORM_DATA(PF_PlatData_MAIN_WND, &mwnd);
		#endif
		
			bool clicked_ok = EXtractoR_Dialog(channels, layers,
												red, green, blue, alpha,
												plugHndl, mwnd);
												
			if(clicked_ok)
			{
				ChannelStatus	*channel_status[4] = {	&extract_status->red,
														&extract_status->green,
														&extract_status->blue,
														&extract_status->alpha};
				for(int i=0; i < 4; i++)
				{
					if(chan_names[i] == "(copy)")
					{
						channel_data[i]->action = DO_COPY;
						channel_status[i]->status = STATUS_NORMAL;
					}
					else
					{
						if(index_map.find(chan_names[i]) != index_map.end())
						{
							channel_data[i]->action = DO_EXTRACT;
							channel_data[i]->index = index_map[chan_names[i]];
							strncpy(channel_data[i]->name, chan_names[i].c_str(), MAX_CHANNEL_NAME_LEN);
							channel_status[i]->status = STATUS_NORMAL;
						}
						else
						{
							channel_data[i]->action = DO_NOTHING;
							channel_status[i]->status = STATUS_ERROR;
						}
					}
				}
				
				params[EXTRACT_DATA]->uu.change_flags = PF_ChangeFlag_CHANGED_VALUE;
			}
		}
		else
		{
			PF_SPRINTF(out_data->return_msg, "No floating point channels available.");
		}
		
		PF_UNLOCK_HANDLE(params[EXTRACT_DATA]->u.arb_d.value);
		PF_UNLOCK_HANDLE(in_data->sequence_data);
	}
	
	return err;
}

DllExport	
PF_Err 
PluginMain (	
	PF_Cmd			cmd,
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output,
	void			*extra)
{
	PF_Err		err = PF_Err_NONE;
	
	try	{
		switch (cmd) {
			case PF_Cmd_ABOUT:
				err = About(in_data,out_data,params,output);
				break;
			case PF_Cmd_GLOBAL_SETUP:
				err = GlobalSetup(in_data,out_data,params,output);
				break;
			case PF_Cmd_PARAMS_SETUP:
				err = ParamsSetup(in_data,out_data,params,output);
				break;
			case PF_Cmd_SEQUENCE_SETUP:
			case PF_Cmd_SEQUENCE_RESETUP:
				err = SequenceSetup(in_data, out_data, params, output);
				break;
			case PF_Cmd_SEQUENCE_FLATTEN:
				err = SequenceFlatten(in_data, out_data, params, output);
				break;
			case PF_Cmd_SEQUENCE_SETDOWN:
				err = SequenceSetdown(in_data, out_data, params, output);
				break;
			case PF_Cmd_SMART_PRE_RENDER:
				err = PreRender(in_data, out_data, (PF_PreRenderExtra*)extra);
				break;
			case PF_Cmd_SMART_RENDER:
				err = SmartRender(in_data, out_data, (PF_SmartRenderExtra*)extra);
				break;
			case PF_Cmd_EVENT:
				err = HandleEvent(in_data, out_data, params, output, (PF_EventExtra	*)extra);
				break;
			case PF_Cmd_DO_DIALOG:
				err = DoDialog(in_data, out_data, params, output);
				break;	
			case PF_Cmd_ARBITRARY_CALLBACK:
				err = HandleArbitrary(in_data, out_data, params, output, (PF_ArbParamsExtra	*)extra);
				break;
		}
	}
	catch(PF_Err &thrown_err) { err = thrown_err; }
	catch(...) { err = PF_Err_INTERNAL_STRUCT_DAMAGED; }
	
	return err;
}
