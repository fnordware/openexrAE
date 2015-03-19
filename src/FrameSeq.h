//
//	OpenEXR file importer/exporter for After Effects (AEIO)
// 
//	by Brendan Bolles <brendan@fnordware.com>
//
//	see OpenEXR.cpp for more information
//

#ifndef FRAMESEQ_H
#define FRAMESEQ_H

#include "AEConfig.h"
#include "AE_IO.h"
#include "AE_Macros.h"
#include "fnord_SuiteHandler.h"

#ifndef MAX
#define MAX(A,B)			( (A) > (B) ? (A) : (B))
#endif
#define AE_RANGE(NUM)		(A_u_short)MIN( MAX( (NUM), 0 ), PF_MAX_CHAN16 )
#define AE8_RANGE(NUM)		(A_u_char)MIN( MAX( (NUM), 0 ), PF_MAX_CHAN8 )
#define SIXTEEN_RANGE(NUM)	(A_u_short)MIN( MAX( (NUM), 0 ), 0xFFFF )

#define AE_TO_FLOAT(NUM)		( (float)(NUM) / (float)PF_MAX_CHAN16 )
#define FLOAT_TO_AE(NUM)		AE_RANGE( ( (NUM) * (float)PF_MAX_CHAN16 ) + 0.5)

#define AE8_TO_FLOAT(NUM)		( (float)(NUM) / (float)PF_MAX_CHAN8 )
#define FLOAT_TO_AE8(NUM)		AE8_RANGE( ( (NUM) * (float)PF_MAX_CHAN8 ) + 0.5)

#define SIXTEEN_TO_FLOAT(NUM)		( (float)(NUM) / (float)0xFFFF )
#define FLOAT_TO_SIXTEEN(NUM)		SIXTEEN_RANGE( ( (NUM) * (float)0xFFFF ) + 0.5)

#ifndef CONVERT16TO8
#define CONVERT16TO8(A)		( (((long)(A) * PF_MAX_CHAN8) + PF_HALF_CHAN16) / PF_MAX_CHAN16)
#endif

// addition to AEIO_AlphaType
#define AEIO_Alpha_UNKNOWN	99

// addition to FIEL_Type
#define FIEL_Type_UNKNOWN	88



#define CHROMATICITIES_MAX_NAME_LEN	255
typedef struct {
	A_char			color_space_name[CHROMATICITIES_MAX_NAME_LEN + 1];
	A_FloatPoint	red;
	A_FloatPoint	green;
	A_FloatPoint	blue;
	A_FloatPoint	white;
} A_Chromaticities;


#define COMPUTER_NAME_SIZE	128
#define COMP_NAME_SIZE		128
typedef struct {
	A_char	proj_name[AEGP_MAX_PROJ_NAME_SIZE];
	A_char	comp_name[COMP_NAME_SIZE];
	A_char	comment_str[AEGP_MAX_RQITEM_COMMENT_SIZE];
	A_char	computer_name[COMPUTER_NAME_SIZE];
	A_char	user_name[COMPUTER_NAME_SIZE];
	A_Time	framerate;
} Render_Info;
	

typedef struct
{
	int hours;
	int minutes;
	int seconds;
	int frame;
	bool dropFrame;
} Time_Code;

typedef struct
{
	A_long			width;
	A_long			height;
	A_long			planes;
	A_long			depth;		// 8 or 16 or 32!
	AEIO_AlphaType	alpha_type;
	A_Ratio			pixel_aspect_ratio;
	PF_Pixel		*premult_color;
	float			framerate;
	FIEL_Label		*field_label;	// if so inclined
	void			*icc_profile;
	size_t			icc_profile_len;
	A_Chromaticities *chromaticities;
	Render_Info		*render_info;
	Time_Code		*time_code;
} FrameSeq_Info;



void
Init_Chromaticities(A_Chromaticities	*chrm);

A_Boolean
Chromaticities_Set(A_Chromaticities	*chrm);

A_Boolean
Profile_To_Chromaticities(
	void				*icc_profile,
	size_t				icc_profile_len,
	A_Chromaticities	*chrm);
	
A_Boolean
Chromaticities_To_Profile(
	void				**icc_profile,
	size_t				*icc_profile_len, 
	A_Chromaticities	*chrm);



A_Err
FrameSeq_Init(struct SPBasicSuite *pica_basicP);

A_Err
FrameSeq_DeathHook(const SPBasicSuite *pica_basicP);

A_Err
FrameSeq_IdleHook(AEIO_BasicData *basic_dataP, AEIO_IdleFlags *idle_flags0);

A_Err
FrameSeq_PurgeHook(const SPBasicSuite *pica_basicP);

A_Err
FrameSeq_ConstructModuleInfo(
	AEIO_ModuleInfo	*info);

A_Err	
FrameSeq_GetInSpecInfo(
	AEIO_BasicData	*basic_dataP,
	AEIO_InSpecH	specH, 
	AEIO_Verbiage	*verbiageP);

A_Err	
FrameSeq_VerifyFileImportable(
	AEIO_BasicData			*basic_dataP,
	AEIO_ModuleSignature	sig, 
	const A_PathType		*file_pathZ,
	A_Boolean				*importablePB);

A_Err	
FrameSeq_InitInSpecFromFile(
	AEIO_BasicData	*basic_dataP,
	const A_PathType		*file_pathZ,
	AEIO_InSpecH	specH);

A_Err	
FrameSeq_DrawSparseFrame(
	AEIO_BasicData					*basic_dataP,
	AEIO_InSpecH					specH, 
	const AEIO_DrawSparseFramePB	*sparse_framePPB, 
	PF_EffectWorld					*wP,
	AEIO_DrawingFlags				*draw_flagsP);
	
A_Err	
FrameSeq_SeqOptionsDlg(
	AEIO_BasicData	*basic_dataP,
	AEIO_InSpecH	specH,  
	A_Boolean		*user_interactedPB0);

A_Err
FrameSeq_DisposeInSpec(
	AEIO_BasicData	*basic_dataP,
	AEIO_InSpecH	specH);

A_Err
FrameSeq_FlattenOptions(
	AEIO_BasicData	*basic_dataP,
	AEIO_InSpecH	specH,
	AEIO_Handle		*flat_optionsPH);

A_Err
FrameSeq_InflateOptions(
	AEIO_BasicData	*basic_dataP,
	AEIO_InSpecH	specH,
	AEIO_Handle		flat_optionsH);

A_Err	
FrameSeq_GetNumAuxChannels(
	AEIO_BasicData	*basic_dataP,
	AEIO_InSpecH	specH,
	A_long			*num_channelsPL);
									
A_Err	
FrameSeq_GetAuxChannelDesc(	
	AEIO_BasicData	*basic_dataP,
	AEIO_InSpecH	specH,
	A_long			chan_indexL,
	PF_ChannelDesc	*descP);
																
A_Err	
FrameSeq_DrawAuxChannel(
	AEIO_BasicData			*basic_dataP,
	AEIO_InSpecH			specH,
	A_long					chan_indexL,
	const AEIO_DrawFramePB	*pbP,
	PF_ChannelChunk			*chunkP);

A_Err	
FrameSeq_FreeAuxChannel(	
	AEIO_BasicData			*basic_dataP,
	AEIO_InSpecH			specH,
	PF_ChannelChunk			*chunkP);


//     input
// =================================
//     output


A_Err	
FrameSeq_GetDepths(
	AEIO_BasicData			*basic_dataP,
	AEIO_OutSpecH			outH, 
	AEIO_SupportedDepthFlags	*which);

A_Err	
FrameSeq_InitOutputSpec(
	AEIO_BasicData			*basic_dataP,
	AEIO_OutSpecH			outH, 
	A_Boolean				*user_interacted);
	
A_Err	
FrameSeq_OutputFrame(
	AEIO_BasicData	*basic_dataP,
	AEIO_OutSpecH			outH, 
	const PF_EffectWorld	*wP);

A_Err	
FrameSeq_UserOptionsDialog(
	AEIO_BasicData		*basic_dataP,
	AEIO_OutSpecH		outH, 
	const PF_EffectWorld	*sample0,
	A_Boolean			*user_interacted0);

A_Err	
FrameSeq_GetOutputInfo(
	AEIO_BasicData		*basic_dataP,
	AEIO_OutSpecH		outH,
	AEIO_Verbiage		*verbiageP);

A_Err	
FrameSeq_DisposeOutputOptions(
	AEIO_BasicData	*basic_dataP,
	void			*optionsPV);

A_Err	
FrameSeq_GetFlatOutputOptions(
	AEIO_BasicData	*basic_dataP,
	AEIO_OutSpecH	outH, 
	AEIO_Handle		*flat_optionsPH);
	
#endif // FRAMESEQ_H
