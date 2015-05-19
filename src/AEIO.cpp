//
//	OpenEXR file importer/exporter for After Effects (AEIO)
// 
//	by Brendan Bolles <brendan@fnordware.com>
//
//	see OpenEXR.cpp for more information
//

#include "AEIO.h"

#include "FrameSeq.h"

#include <assert.h>

AEGP_PluginID			S_mem_id					=	0;


static A_Err DeathHook(	
	AEGP_GlobalRefcon unused1,
	AEGP_DeathRefcon deathcon)
{
	return FrameSeq_DeathHook((const SPBasicSuite *)deathcon);
}

static A_Err CommandHook(
	AEGP_GlobalRefcon	unused1,
	AEGP_CommandRefcon	refcon,
	AEGP_Command		command,
	AEGP_HookPriority	hook_priority,
	A_Boolean			already_handledB,
	A_Boolean			*handledPB)
{
#define CMD_PURGE_IMAGE_CACHES	2372
#define CMD_PURGE_ALL			2373
#define CMD_PURGE_ALL_PLUS_DISK 10200

	if(command == CMD_PURGE_IMAGE_CACHES || command == CMD_PURGE_ALL || command == CMD_PURGE_ALL_PLUS_DISK)
	{
		return FrameSeq_PurgeHook((const SPBasicSuite *)refcon);
	}
	
	return A_Err_NONE;
}

static A_Err	
AEIO_InitInSpecFromFile(
	AEIO_BasicData	*basic_dataP,
	const A_PathType	*file_pathZ,
	AEIO_InSpecH	specH)
{ 
	// handled by FrameSeq
	return FrameSeq_InitInSpecFromFile(basic_dataP, file_pathZ, specH);
}

static A_Err
AEIO_InitInSpecInteractive(
	AEIO_BasicData	*basic_dataP,
	AEIO_InSpecH	specH)
{
	assert(FALSE); // don't think I should be getting this
	
	return A_Err_NONE; 
}

static A_Err
AEIO_DisposeInSpec(
	AEIO_BasicData	*basic_dataP,
	AEIO_InSpecH	specH)
{ 
	// FrameSeq
	return FrameSeq_DisposeInSpec(basic_dataP, specH);
}

static A_Err
AEIO_FlattenOptions(
	AEIO_BasicData	*basic_dataP,
	AEIO_InSpecH	specH,
	AEIO_Handle		*flat_optionsPH)
{ 
	// FrameSeq
	return FrameSeq_FlattenOptions(basic_dataP, specH, flat_optionsPH);
}	

static A_Err
AEIO_InflateOptions(
	AEIO_BasicData	*basic_dataP,
	AEIO_InSpecH	specH,
	AEIO_Handle		flat_optionsH)
{ 
	// FrameSeq
	return FrameSeq_InflateOptions(basic_dataP, specH, flat_optionsH);
}

static A_Err
AEIO_SynchInSpec(
	AEIO_BasicData	*basic_dataP,
	AEIO_InSpecH	specH, 
	A_Boolean		*changed0)
{ 
	// called when project is loaded
	
	return AEIO_Err_USE_DFLT_CALLBACK;
}

static A_Err	
AEIO_GetActiveExtent(
	AEIO_BasicData	*basic_dataP,
	AEIO_InSpecH	specH,				/* >> */
	const A_Time	*tr,				/* >> */
	A_LRect			*extent)			/* << */
{ 
	assert(FALSE); // don't think I should be getting this
	
	return AEIO_Err_USE_DFLT_CALLBACK; 
}

static A_Err	
AEIO_GetInSpecInfo(
	AEIO_BasicData	*basic_dataP,
	AEIO_InSpecH	specH, 
	AEIO_Verbiage	*verbiageP)
{ 
	// handled by FrameSeq framework
	return FrameSeq_GetInSpecInfo(basic_dataP, specH, verbiageP);
}

static A_Err	
AEIO_DrawSparseFrame(
	AEIO_BasicData					*basic_dataP,
	AEIO_InSpecH					specH, 
	const AEIO_DrawSparseFramePB	*sparse_framePPB, 
	PF_EffectWorld						*wP,
	AEIO_DrawingFlags				*draw_flagsP)
{ 
	// handled by the FrameSeq framework
	return FrameSeq_DrawSparseFrame(basic_dataP, specH, sparse_framePPB, wP, draw_flagsP);
}

static A_Err	
AEIO_GetDimensions(
	AEIO_BasicData			*basic_dataP,
	AEIO_InSpecH			 specH, 
	const AEIO_RationalScale *rs0,
	A_long					 *width0, 
	A_long					 *height0)
{ 
	assert(FALSE); // don't think I should be getting this
	
	return AEIO_Err_USE_DFLT_CALLBACK; 
}
					
static A_Err	
AEIO_GetDuration(
	AEIO_BasicData	*basic_dataP,
	AEIO_InSpecH	specH, 
	A_Time			*tr)
{ 
	assert(FALSE); // don't think I should be getting this
	
	return AEIO_Err_USE_DFLT_CALLBACK; 
}

static A_Err	
AEIO_GetTime(
	AEIO_BasicData	*basic_dataP,
	AEIO_InSpecH	specH, 
	A_Time			*tr)
{ 
	assert(FALSE); // don't think I should be getting this
	
	return AEIO_Err_USE_DFLT_CALLBACK; 
}

static A_Err	
AEIO_GetSound(
	AEIO_BasicData				*basic_dataP,	
	AEIO_InSpecH				specH,
	AEIO_SndQuality				quality,
	const AEIO_InterruptFuncs	*interrupt_funcsP0,	
	const A_Time				*startPT,	
	const A_Time				*durPT,	
	A_u_long					start_sampLu,
	A_u_long					num_samplesLu,
	void						*dataPV)
{ 
	assert(FALSE); // don't think I should be getting this
	
	return A_Err_NONE;
}

static A_Err	
AEIO_InqNextFrameTime(
	AEIO_BasicData			*basic_dataP,
	AEIO_InSpecH			specH, 
	const A_Time			*base_time_tr,
	AEIO_TimeDir			time_dir, 
	A_Boolean				*found0,
	A_Time					*key_time_tr0)
{ 
	return AEIO_Err_USE_DFLT_CALLBACK; 
}

static A_Err	
AEIO_DisposeOutputOptions(
	AEIO_BasicData	*basic_dataP,
	void			*optionsPV) // what the...?
{ 
	return FrameSeq_DisposeOutputOptions(basic_dataP, optionsPV);
}

static A_Err	
AEIO_UserOptionsDialog(
	AEIO_BasicData		*basic_dataP,
	AEIO_OutSpecH		outH, 
	const PF_EffectWorld	*sample0,
	A_Boolean			*user_interacted0)
{ 
	return FrameSeq_UserOptionsDialog(basic_dataP, outH, sample0, user_interacted0);
}

static A_Err	
AEIO_GetOutputInfo(
	AEIO_BasicData		*basic_dataP,
	AEIO_OutSpecH		outH,
	AEIO_Verbiage		*verbiageP)
{ 
	return FrameSeq_GetOutputInfo(basic_dataP, outH, verbiageP);
}

static A_Err	
AEIO_SetOutputFile(
	AEIO_BasicData	*basic_dataP,
	AEIO_OutSpecH	outH, 
#if PF_AE_PLUG_IN_VERSION < PF_AE100_PLUG_IN_VERSION
	A_PathType	*file_pathZ)
#else
	const A_UTF16Char	*file_pathZ)
#endif
{ 
	// this does get called, will have the full "file.[####].exr" path
  	return AEIO_Err_USE_DFLT_CALLBACK;
}

static A_Err	
AEIO_StartAdding(
	AEIO_BasicData		*basic_dataP,
	AEIO_OutSpecH		outH, 
	A_long				flags)
{ 
	// this does actually get called, if we need to prepare
	return A_Err_NONE;
}

static A_Err	
AEIO_AddFrame(
	AEIO_BasicData			*basic_dataP,
	AEIO_OutSpecH			outH, 
	A_long					frame_index, 
	A_long					frames,
	const PF_EffectWorld	*wP, 
	const A_LPoint			*origin0,
	A_Boolean				was_compressedB,	
	AEIO_InterruptFuncs		*inter0)
{ 
	assert(FALSE); // shouldn't get called for frame-based formats
	
	return A_Err_NONE;
}
								
static A_Err	
AEIO_EndAdding(
	AEIO_BasicData	*basic_dataP,
	AEIO_OutSpecH			outH, 
	A_long					flags)
{ 
	// done with the render, anything to dispose?
	return A_Err_NONE;
}

static A_Err	
AEIO_OutputFrame(
	AEIO_BasicData	*basic_dataP,
	AEIO_OutSpecH			outH, 
	const PF_EffectWorld	*wP)
{
	// FrameSeq 
	return FrameSeq_OutputFrame(basic_dataP, outH, wP);
}

static A_Err	
AEIO_WriteLabels(
	AEIO_BasicData	*basic_dataP,
	AEIO_OutSpecH	outH, 
	AEIO_LabelFlags	*written)
{ 
	// yep, this is getting called, once per frame
	return AEIO_Err_USE_DFLT_CALLBACK;
}

static A_Err	
AEIO_GetSizes(
	AEIO_BasicData	*basic_dataP,
	AEIO_OutSpecH	outH, 
	A_u_longlong	*free_space, 
	A_u_longlong	*file_size)
{ 
	// this gets called, but default callback should be fine
	return AEIO_Err_USE_DFLT_CALLBACK;
}

static A_Err	
AEIO_Flush(
	AEIO_BasicData	*basic_dataP,
	AEIO_OutSpecH	outH)
{
	/*	free any temp buffers you kept around for
		writing.  Yes, this gets called.
	*/
	return A_Err_NONE; 
}

static A_Err	
AEIO_AddSoundChunk(
	AEIO_BasicData	*basic_dataP,
	AEIO_OutSpecH	outH, 
	const A_Time	*start, 
	A_u_long	num_samplesLu,
	const void		*dataPV)
{ 
	assert(FALSE); // shouldn't get called for frame-based formats
	
	return A_Err_NONE;
}


static A_Err	
AEIO_Idle(
	AEIO_BasicData			*basic_dataP,
	AEIO_ModuleSignature	sig,
	AEIO_IdleFlags			*idle_flags0)
{
	assert(sig == 'oEXR' || sig == 'RXEo'); // some sort of historical endian thing?

	return FrameSeq_IdleHook(basic_dataP, idle_flags0);
}

static A_Err	
AEIO_GetDepths(
	AEIO_BasicData			*basic_dataP,
	AEIO_OutSpecH			outH, 
	AEIO_SupportedDepthFlags	*which)
{ 
	// yeah, FrameSeq
	return FrameSeq_GetDepths(basic_dataP, outH, which);
}

static A_Err	
AEIO_GetOutputSuffix(
	AEIO_BasicData	*basic_dataP,
	AEIO_OutSpecH	outH, 
	A_char			*suffix)
{ 
	return AEIO_Err_USE_DFLT_CALLBACK;
}

static A_Err	
AEIO_SeqOptionsDlg(
	AEIO_BasicData	*basic_dataP,
	AEIO_InSpecH	specH,  
	A_Boolean		*user_interactedPB0)
{ 
	// FrameSeq got dis
	return FrameSeq_SeqOptionsDlg(basic_dataP, specH, user_interactedPB0);
}

static A_Err	
AEIO_GetNumAuxChannels(
	AEIO_BasicData	*basic_dataP,
	AEIO_InSpecH	specH,
	A_long			*num_channelsPL)
{ 
	return FrameSeq_GetNumAuxChannels(basic_dataP, specH, num_channelsPL);
}
									
static A_Err	
AEIO_GetAuxChannelDesc(	
	AEIO_BasicData	*basic_dataP,
	AEIO_InSpecH	specH,
	A_long			chan_indexL,
	PF_ChannelDesc	*descP)
{ 
	return FrameSeq_GetAuxChannelDesc(basic_dataP, specH, chan_indexL, descP);	
}
																
static A_Err	
AEIO_DrawAuxChannel(
	AEIO_BasicData	*basic_dataP,
	AEIO_InSpecH			specH,
	A_long					chan_indexL,
	const AEIO_DrawFramePB	*pbP,
	PF_ChannelChunk			*chunkP)
{ 
	return FrameSeq_DrawAuxChannel(basic_dataP, specH, chan_indexL, pbP, chunkP);
}

static A_Err	
AEIO_FreeAuxChannel(	
	AEIO_BasicData	*basic_dataP,
	AEIO_InSpecH			specH,
	PF_ChannelChunk			*chunkP)
{ 
	return FrameSeq_FreeAuxChannel(basic_dataP, specH, chunkP);
}

static A_Err	
AEIO_NumAuxFiles(		
	AEIO_BasicData	*basic_dataP,
	AEIO_InSpecH	seqH,
	A_long			*files_per_framePL0)
{ 
	*files_per_framePL0 = 0;
	
	return A_Err_NONE; 
}

static A_Err	
AEIO_GetNthAuxFileSpec(
	AEIO_BasicData	*basic_dataP,
	AEIO_InSpecH			seqH,
	A_long					frame_numL, 
	A_long					n,
#if PF_AE_PLUG_IN_VERSION < PF_AE100_PLUG_IN_VERSION
	A_PathType				*file_pathZ)
#else
	AEGP_MemHandle			*pathPH)
#endif
{
	assert(FALSE); // no aux files
	
	return A_Err_NONE; 
}

static A_Err	
AEIO_CloseSourceFiles(
	AEIO_BasicData	*basic_dataP,
	AEIO_InSpecH			seqH)
{
	// this well get called during Edit->Purge->All Memory, for some reason?
	// (frame-based formats don't usually keep files open)

	return A_Err_NONE; 
}

static A_Err	
AEIO_CountUserData(
	AEIO_BasicData	*basic_dataP,
	AEIO_InSpecH			inH,
	A_u_long 				typeLu,
	A_u_long				max_sizeLu,
	A_u_long				*num_of_typePLu)
{
	assert(FALSE); // don't think I should be getting this

	return A_Err_NONE; 
}

static A_Err	
AEIO_SetUserData(    
	AEIO_BasicData	*basic_dataP,
	AEIO_OutSpecH			outH,
	A_u_long				typeLu,
	A_u_long				indexLu,
	const AEIO_Handle		dataH)
{ 
	assert(FALSE); // don't think I should be getting this

	return A_Err_NONE; 
}

static A_Err	
AEIO_GetUserData(   
	AEIO_BasicData	*basic_dataP,
	AEIO_InSpecH			inH,
	A_u_long 				typeLu,
	A_u_long				indexLu,
	A_u_long				max_sizeLu,
	AEIO_Handle				*dataPH)
{ 
	assert(FALSE); // don't think I should be getting this
	
	return A_Err_NONE; 
}
                            
static A_Err	
AEIO_AddMarker(		
	AEIO_BasicData	*basic_dataP,
	AEIO_OutSpecH 			outH, 
	A_long 					frame_index, 
	AEIO_MarkerType 		marker_type, 
	void					*marker_dataPV, 
	AEIO_InterruptFuncs		*inter0)
{
	assert(FALSE);  // don't expect to get this called
	/*	The type of the marker is in marker_type,
		and the text they contain is in 
		marker_dataPV.
	*/
	return A_Err_NONE; 
}

static A_Err	
AEIO_VerifyFileImportable(
	AEIO_BasicData			*basic_dataP,
	AEIO_ModuleSignature	sig, 
	const A_PathType		*file_pathZ, 
	A_Boolean				*importablePB)
{ 
	// pass along to FrameSeq
	return FrameSeq_VerifyFileImportable(basic_dataP, sig, file_pathZ, importablePB);
}	
	
static A_Err	
AEIO_InitOutputSpec(
	AEIO_BasicData			*basic_dataP,
	AEIO_OutSpecH			outH, 
	A_Boolean				*user_interacted)
{
	// yep, FrameSeq
	return FrameSeq_InitOutputSpec(basic_dataP, outH, user_interacted);
}

static A_Err	
AEIO_OutputInfoChanged(
	AEIO_BasicData		*basic_dataP,
	AEIO_OutSpecH		outH)
{
	// different options will just result in different options data, that's all
	return A_Err_NONE;
}

static A_Err	
AEIO_GetFlatOutputOptions(
	AEIO_BasicData	*basic_dataP,
	AEIO_OutSpecH	outH, 
	AEIO_Handle		*optionsPH)
{
	// FrameSeq
	return FrameSeq_GetFlatOutputOptions(basic_dataP, outH, optionsPH);
}

static A_Err
AEIO_ConstructModuleInfo(
	AEIO_ModuleInfo	*info)
{
	// handled by FrameSeq framework
	return FrameSeq_ConstructModuleInfo(info);
}


static A_Err
AEIO_ConstructFunctionBlock(
	Current_AEIO_FunctionBlock	*funcs)
{
	if (funcs)
	{
		funcs->AEIO_AddFrame				=	AEIO_AddFrame;
		funcs->AEIO_AddMarker				=	AEIO_AddMarker;
		funcs->AEIO_AddSoundChunk			=	AEIO_AddSoundChunk;
		funcs->AEIO_CloseSourceFiles		=	AEIO_CloseSourceFiles;
		funcs->AEIO_CountUserData			=	AEIO_CountUserData;
		funcs->AEIO_DisposeInSpec			=	AEIO_DisposeInSpec;
		funcs->AEIO_DisposeOutputOptions	=	AEIO_DisposeOutputOptions;
		funcs->AEIO_DrawAuxChannel			=	AEIO_DrawAuxChannel;
		funcs->AEIO_DrawSparseFrame			=	AEIO_DrawSparseFrame;
		funcs->AEIO_EndAdding				=	AEIO_EndAdding;
		funcs->AEIO_FlattenOptions			=	AEIO_FlattenOptions;
		funcs->AEIO_Flush					=	AEIO_Flush;
		funcs->AEIO_FreeAuxChannel			=	AEIO_FreeAuxChannel;
		funcs->AEIO_GetActiveExtent			=	AEIO_GetActiveExtent;
		funcs->AEIO_GetAuxChannelDesc		=	AEIO_GetAuxChannelDesc;
		funcs->AEIO_GetDepths				=	AEIO_GetDepths;
		funcs->AEIO_GetDimensions			=	AEIO_GetDimensions;
		funcs->AEIO_GetDuration				=	AEIO_GetDuration;
		funcs->AEIO_GetInSpecInfo			=	AEIO_GetInSpecInfo;
		funcs->AEIO_GetNthAuxFileSpec		=	AEIO_GetNthAuxFileSpec;
		funcs->AEIO_GetNumAuxChannels		=	AEIO_GetNumAuxChannels;
		funcs->AEIO_GetOutputInfo			=	AEIO_GetOutputInfo;
		funcs->AEIO_GetOutputSuffix			=	AEIO_GetOutputSuffix;
		funcs->AEIO_GetSizes				=	AEIO_GetSizes;
		funcs->AEIO_GetSound				=	AEIO_GetSound;
		funcs->AEIO_GetTime					=	AEIO_GetTime;
		funcs->AEIO_GetUserData				=	AEIO_GetUserData;
		funcs->AEIO_Idle					=	AEIO_Idle;
		funcs->AEIO_InflateOptions			=	AEIO_InflateOptions;
		funcs->AEIO_InitInSpecFromFile		=	AEIO_InitInSpecFromFile;
		funcs->AEIO_InitInSpecInteractive	=	AEIO_InitInSpecInteractive;
		funcs->AEIO_InqNextFrameTime		=	AEIO_InqNextFrameTime;
		funcs->AEIO_NumAuxFiles				=	AEIO_NumAuxFiles;
		funcs->AEIO_OutputFrame				=	AEIO_OutputFrame;
		funcs->AEIO_SeqOptionsDlg			=	AEIO_SeqOptionsDlg;
		funcs->AEIO_SetOutputFile			=	AEIO_SetOutputFile;
		funcs->AEIO_SetUserData				=	AEIO_SetUserData;
		funcs->AEIO_StartAdding				=	AEIO_StartAdding;
		funcs->AEIO_SynchInSpec				=	AEIO_SynchInSpec;
		funcs->AEIO_UserOptionsDialog		=	AEIO_UserOptionsDialog;
		funcs->AEIO_VerifyFileImportable	=	AEIO_VerifyFileImportable;
		funcs->AEIO_WriteLabels				=	AEIO_WriteLabels;
		funcs->AEIO_InitOutputSpec			=	AEIO_InitOutputSpec;
		funcs->AEIO_GetFlatOutputOptions	=	AEIO_GetFlatOutputOptions;
		funcs->AEIO_OutputInfoChanged		=	AEIO_OutputInfoChanged;

		return A_Err_NONE;
	}
	else
	{
		return A_Err_STRUCT;
	}
}


A_Err
GPMain_IO(
	struct SPBasicSuite		*pica_basicP,			/* >> */
	A_long				 	major_versionL,			/* >> */		
	A_long					minor_versionL,			/* >> */
#if PF_AE_PLUG_IN_VERSION < PF_AE100_PLUG_IN_VERSION
	const A_char		 	*file_pathZ,			/* >> platform-specific delimiters */
	const A_char		 	*res_pathZ,				/* >> platform-specific delimiters */
#endif
	AEGP_PluginID			aegp_plugin_id,			/* >> */
	void					*global_refconPV)		/* << */
{
	A_Err 				err					= A_Err_NONE;
	AEIO_ModuleInfo		info;
	Current_AEIO_FunctionBlock	funcs;
	AEGP_SuiteHandler	suites(pica_basicP);	

	S_mem_id = aegp_plugin_id;
	
	
	if (!err)
	{
		err = AEIO_ConstructModuleInfo(&info);
	}
	if (!err)
	{
		err = AEIO_ConstructFunctionBlock(&funcs);
	}			
	if (!err)
	{
		err = suites.RegisterSuite()->AEGP_RegisterIO(	aegp_plugin_id,
														NULL,
														&info, 
														&funcs);
	}
	if (!err)
	{
		err = FrameSeq_Init(pica_basicP); // plug-in specific initialization
	}
	if (!err)
	{
		err = suites.RegisterSuite()->AEGP_RegisterDeathHook(aegp_plugin_id,
															DeathHook,
															(AEGP_DeathRefcon)pica_basicP);
	}		
	if (!err)
	{
		err = suites.RegisterSuite()->AEGP_RegisterCommandHook(aegp_plugin_id,
																AEGP_HP_AfterAE,
																CMD_PURGE_IMAGE_CACHES,
																CommandHook,
																(AEGP_CommandRefcon)pica_basicP);
	}		
	if (!err)
	{
		err = suites.RegisterSuite()->AEGP_RegisterCommandHook(aegp_plugin_id,
																AEGP_HP_AfterAE,
																CMD_PURGE_ALL,
																CommandHook,
																(AEGP_CommandRefcon)pica_basicP);
	}		
	if (!err)
	{
		err = suites.RegisterSuite()->AEGP_RegisterCommandHook(aegp_plugin_id,
                                                               AEGP_HP_AfterAE,
                                                               CMD_PURGE_ALL_PLUS_DISK,
                                                               CommandHook,
                                                               (AEGP_CommandRefcon)pica_basicP);
	}		
    
	return err;
}

