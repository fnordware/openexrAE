//
//	OpenEXR file importer/exporter for After Effects (AEIO)
// 
//	by Brendan Bolles <brendan@fnordware.com>
//
//	see OpenEXR.cpp for more information
//

#pragma once 


#ifdef MSWindows
#include <windows.h>
#endif

#include "AEConfig.h"
#include "entry.h"
#include "AE_IO.h"
#include "AE_Macros.h"
#include "AE_EffectCBSuites.h"
#include "fnord_SuiteHandler.h"

#ifdef __cplusplus
extern "C" {
#endif


DllExport A_Err
GPMain_IO(
	struct SPBasicSuite		*pica_basicP,			/* >> */
	A_long				 	major_versionL,			/* >> */		
	A_long					minor_versionL,			/* >> */		
#if PF_AE_PLUG_IN_VERSION < PF_AE100_PLUG_IN_VERSION
	const A_char		 	*file_pathZ,				/* >> platform-specific delimiters */
	const A_char		 	*res_pathZ,				/* >> platform-specific delimiters */
#endif
	AEGP_PluginID			aegp_plugin_id,			/* >> */
	void					*global_refconPV);			/* << */


#ifdef __cplusplus
}
#endif

