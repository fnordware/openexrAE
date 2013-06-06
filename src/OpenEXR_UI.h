//
//	OpenEXR file importer/exporter for After Effects (AEIO)
// 
//	by Brendan Bolles <brendan@fnordware.com>
//
//	see OpenEXR.cpp for more information
//

#ifndef OpenEXR_UI_H
#define OpenEXR_UI_H


#include "OpenEXR.h"


A_Err
OpenEXR_displayWindowDialog(
	AEIO_BasicData		*basic_dataP,
	const char			*dialogMessage,
	const char			*displayWindowText,
	const char			*dataWindowText,
	A_Boolean			*displayWindow,
	A_Boolean			*displayWindowDefault,
	A_Boolean			*autoShowDialog,
	A_Boolean			*user_interactedPB0);

A_Err	
OpenEXR_InDialog(
	AEIO_BasicData		*basic_dataP,
	A_Boolean			*cache_channels,
	A_long				*num_caches,
	A_Boolean			*user_interactedPB0);

A_Err	
OpenEXR_OutDialog(
	AEIO_BasicData		*basic_dataP,
	OpenEXR_outData		*options,
	A_Boolean			*user_interactedPB0);

void
OpenEXR_CopyPluginPath(
	A_char				*pathZ,
	A_long				max_len);
	
bool
ShiftKeyHeld();

#endif // OpenEXR_UI_H