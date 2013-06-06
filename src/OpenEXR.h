//
//	OpenEXR file importer/exporter for After Effects (AEIO)
// 
//	by Brendan Bolles <brendan@fnordware.com>
//
//	see OpenEXR.cpp for more information
//

#ifndef OpenEXR_H
#define OpenEXR_H

#include "FrameSeq.h"


#define PLUGIN_NAME		"OpenEXR"


enum {
	DW_DATA_WINDOW = 0,
	DW_DISPLAY_WINDOW,
	DW_UNKNOWN
};
typedef A_u_char DisplayWindow;


typedef struct OpenEXR_inData
{
	A_u_char			compression_type;
	A_Boolean			cache_channels;
	DisplayWindow		display_window;
	A_u_char			nothing; // reserved for byte alignment
	A_u_long			real_channels;
	A_u_char			reserved[24]; // total of 32 bytes at this point
	A_u_long			channels;
	PF_ChannelDesc		channel[1];
} OpenEXR_inData;



typedef struct OpenEXR_outData
{
	A_u_char	compression_type;
	A_Boolean	float_not_half;
	A_Boolean	luminance_chroma;
	char		reserved[61]; // total of 64 bytes
} OpenEXR_outData;


A_Err
OpenEXR_Init(struct SPBasicSuite *pica_basicP);

A_Err
OpenEXR_DeathHook(const SPBasicSuite *pica_basicP);

A_Err
OpenEXR_IdleHook(const SPBasicSuite *pica_basicP);

A_Err
OpenEXR_PurgeHook(const SPBasicSuite *pica_basicP);


A_Err
OpenEXR_ConstructModuleInfo(
	AEIO_ModuleInfo	*info);

A_Err	
OpenEXR_GetInSpecInfo(
	const A_PathType	*file_pathZ,
	OpenEXR_inData		*options,
	AEIO_Verbiage		*verbiageP);

A_Err	
OpenEXR_VerifyFile(
	const A_PathType		*file_pathZ, 
	A_Boolean				*importablePB);

A_Err
OpenEXR_FileInfo(
	AEIO_BasicData		*basic_dataP,
	const A_PathType	*file_pathZ,
	FrameSeq_Info		*info,
	AEIO_Handle			optionsH);

A_Err	
OpenEXR_DrawSparseFrame(
	AEIO_BasicData					*basic_dataP,
	const AEIO_DrawSparseFramePB	*sparse_framePPB, 
	PF_EffectWorld					*wP,
	AEIO_DrawingFlags				*draw_flagsP,
	const A_PathType				*file_pathZ,
	FrameSeq_Info					*info,
	OpenEXR_inData					*options);
	

A_Err	
OpenEXR_InitInOptions(
	AEIO_BasicData	*basic_dataP,
	OpenEXR_inData	*options);

A_Err	
OpenEXR_CopyInOptions(
	AEIO_BasicData	*basic_dataP,
	OpenEXR_inData	*new_options,
	const OpenEXR_inData	*old_options);

A_Err	
OpenEXR_ReadOptionsDialog(
	AEIO_BasicData	*basic_dataP,
	OpenEXR_inData	*options,
	A_Boolean		*user_interactedPB0);
	
A_Err
OpenEXR_FlattenInputOptions(
	AEIO_BasicData	*basic_dataP,
	AEIO_Handle		optionsH);

A_Err
OpenEXR_InflateInputOptions(
	AEIO_BasicData	*basic_dataP,
	AEIO_Handle		optionsH);


A_Err	
OpenEXR_GetNumAuxChannels(
	AEIO_BasicData		*basic_dataP,
	AEIO_Handle			optionsH,
	const A_PathType	*file_pathZ,
	A_long				*num_channelsPL);
									
A_Err	
OpenEXR_GetAuxChannelDesc(	
	AEIO_BasicData		*basic_dataP,
	AEIO_Handle			optionsH,
	const A_PathType	*file_pathZ,
	A_long				chan_indexL,
	PF_ChannelDesc		*descP);
																
A_Err	
OpenEXR_DrawAuxChannel(
	AEIO_BasicData		*basic_dataP,
	OpenEXR_inData		*options,
	const A_PathType	*file_pathZ,
	A_long				chan_indexL,
	const AEIO_DrawFramePB	*pbP,
	PF_Point			scale,
	PF_ChannelChunk		*chunkP);


//     input
// =================================
//     output


A_Err	
OpenEXR_GetDepths(
	AEIO_SupportedDepthFlags		*which);
	

A_Err	
OpenEXR_InitOutOptions(
	OpenEXR_outData	*options);

A_Err
OpenEXR_OutputFile(
	AEIO_BasicData		*basic_dataP,
	const A_PathType	*file_pathZ,
	FrameSeq_Info		*info,
	OpenEXR_outData	*options,
	PF_EffectWorld		*wP);

A_Err	
OpenEXR_WriteOptionsDialog(
	AEIO_BasicData		*basic_dataP,
	OpenEXR_outData	*options,
	A_Boolean			*user_interactedPB0);

A_Err	
OpenEXR_GetOutSpecInfo(
	const A_PathType	*file_pathZ,
	OpenEXR_outData		*options,
	AEIO_Verbiage		*verbiageP);

A_Err
OpenEXR_FlattenOutputOptions(
	OpenEXR_outData	*options);

A_Err
OpenEXR_InflateOutputOptions(
	OpenEXR_outData	*options);
	
	
#endif // OpenEXR_H
