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

#pragma once

#ifndef _EXTRACTOR_H_
#define _EXTRACTOR_H_


#define PF_DEEP_COLOR_AWARE 1

#include "AEConfig.h"
#include "entry.h"
#include "AE_Effect.h"
#include "AE_EffectCB.h"
#include "AE_Macros.h"
#include "Param_Utils.h"
#include "AE_ChannelSuites.h"
#include "AE_EffectCBSuites.h"
#include "String_Utils.h"
#include "AE_GeneralPlug.h"
#include "fnord_SuiteHandler.h"

#ifdef MSWindows
	#include <Windows.h>
#else 
	#ifndef __MACH__
		#include "string.h"
	#endif
#endif	


// Versioning information 

#define NAME				"EXtractoR"
#define DESCRIPTION			"Extract 3D Channels"
#define RELEASE_DATE		__DATE__
#define AUTHOR				"Brendan Bolles"
#define COPYRIGHT			"(c) 2007-2013 fnord"
#define WEBSITE				"www.fnordware.com"
#define	MAJOR_VERSION		1
#define	MINOR_VERSION		8
#define	BUG_VERSION			0
#define	STAGE_VERSION		PF_Stage_RELEASE
#define	BUILD_VERSION		0


enum {
	EXTRACT_INPUT = 0,
		EXTRACT_DATA,
	EXTRACT_PROCESS_START,
		EXTRACT_BLACK,
		EXTRACT_WHITE,
		EXTRACT_UNMULT,
	EXTRACT_PROCESS_END,
	
	EXTRACT_NUM_PARAMS
};

enum {
	RED_ID_OBSOLETE = 1,
	GREEN_ID_OBSOLETE,
	BLUE_ID_OBSOLETE,
	ALPHA_ID_OBSOLETE,
	
	BLACK_ID,
	WHITE_ID,
	UNMULT_ID,
	
	INFO_ID_OBSOLETE,
	
	CHANNELS_START_ID_OBSOLETE,
	CHANNELS_END_ID_OBSOLETE,
	PROCESS_START_ID,
	PROCESS_END_ID,
	
	ARBITRARY_DATA_ID
};

#define MAX_CHANNEL_NAME_LEN 127 // not sure why I used this instead of PF_CHANNEL_NAME_LEN (63)

enum {
	STATUS_UNKNOWN = 0,
	STATUS_NORMAL,
	STATUS_INDEX_CHANGE,
	STATUS_NOT_FOUND,
	STATUS_NO_CHANNELS,
	STATUS_ERROR
};
typedef A_char	Status;

typedef struct {
	Status			status;
	A_long			index; // in case of STATUS_INDEX_CHANGE
} ChannelStatus;

typedef struct {
	ChannelStatus	red;
	ChannelStatus	green;
	ChannelStatus	blue;
	ChannelStatus	alpha;
	ChannelStatus	compound;
} ExtractStatus;


enum {
	DO_EXTRACT = 1,
	DO_COPY,
	DO_FULL_ON,
	DO_FULL_OFF,
	DO_NOTHING
};
typedef A_char	ExtractAction;

typedef struct {
	ExtractAction	action;
	A_long			index; // 0-based index in the file
	char			reserved[27]; // total of 32 bytes up to here
	A_char			name[MAX_CHANNEL_NAME_LEN+1];
} ChannelData;

#define CURRENT_ARB_VERSION 1

typedef struct {
	A_u_char		version; // version of this data structure
	A_Boolean		single_channel;
	char			reserved[30]; // total of 32 bytes up to here
	ChannelData		red;
	ChannelData		green;
	ChannelData		blue;
	ChannelData		alpha;
} ArbitraryData;

#define ARB_REFCON			(void*)0xDEADBEAF


// UI drawing constants
#define kUI_ARROW_MARGIN	20
#define kUI_RIGHT_MARGIN	5
#define kUI_TOP_MOVEDOWN	11
#define kUI_BOTTOM_MOVEUP	0

#ifdef MAC_ENV
#define kUI_CONTROL_DOWN	10
#define kUI_CONTROL_UP		10
#else
	#if PF_AE_PLUG_IN_VERSION >= PF_AE100_PLUG_IN_VERSION
	#define kUI_CONTROL_DOWN	10
	#define kUI_CONTROL_UP		10
	#else
	#define kUI_CONTROL_DOWN	0
	#define kUI_CONTROL_UP		0
	#endif
#endif
#define kUI_CONTROL_STEP	15


#define kUI_CONTROL_RIGHT	8
#define kUI_CONTROL_PROP_PADDING	16

#if PF_AE_PLUG_IN_VERSION >= PF_AE100_PLUG_IN_VERSION
#define kUI_TITLE_COLOR_SCALEDOWN	0.15
#else
#define kUI_TITLE_COLOR_SCALEDOWN	0.3
#endif

#define TITLE_COMP(COMP)	(65535 * kUI_TITLE_COLOR_SCALEDOWN) + ((COMP) * (1 - kUI_TITLE_COLOR_SCALEDOWN) )

#define DOWN_PLACE(NUM, OR)	(OR) + kUI_CONTROL_DOWN + (kUI_CONTROL_STEP * (NUM))
#define RIGHT_STATUS(OR)	(OR) + kUI_CONTROL_RIGHT
#define RIGHT_PROP(OR)		(OR) + kUI_CONTROL_RIGHT + kUI_CONTROL_PROP_PADDING


enum { //Info elements
	INFO_RED = 0,
	INFO_GREEN,
	INFO_BLUE,
	INFO_ALPHA,
	
	INFO_TOTAL_ITEMS
};

#define kUI_CONTROL_HEIGHT	(kUI_CONTROL_DOWN + (kUI_CONTROL_STEP * INFO_TOTAL_ITEMS) - kUI_CONTROL_UP)
#define kUI_CONTROL_WIDTH	0

#ifdef __cplusplus
	extern "C" {
#endif


// Prototypes

DllExport	PF_Err 
PluginMain (	
	PF_Cmd			cmd,
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output,
	void			*extra) ;


PF_Err 
DoDialog (
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output );


PF_Err
HandleEvent ( 
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output,
	PF_EventExtra	*extra );


PF_Err
ArbNewDefault( // needed by ParamSetup()
	PF_InData			*in_data,
	PF_OutData			*out_data,
	void				*refconPV,
	PF_ArbitraryH		*arbPH);

PF_Err 
HandleArbitrary(
	PF_InData			*in_data,
	PF_OutData			*out_data,
	PF_ParamDef			*params[],
	PF_LayerDef			*output,
	PF_ArbParamsExtra	*extra);


#if defined(MAC_ENV) && PF_AE_PLUG_IN_VERSION >= PF_AE100_PLUG_IN_VERSION
void SetMickeyCursor(); // love our Mickey cursor, but we need an Objectice-C call in Cocoa
#endif

#ifdef __cplusplus
	}
#endif



#endif // _EXTRACTOR_H_