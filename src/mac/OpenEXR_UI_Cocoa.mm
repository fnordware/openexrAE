//
//	OpenEXR file importer/exporter for After Effects (AEIO)
// 
//	by Brendan Bolles <brendan@fnordware.com>
//
//	see OpenEXR.cpp for more information
//

#include "AEConfig.h"

#include "OpenEXR_UI.h"

#import "OpenEXR_displayWindow_Controller.h"
#import "OpenEXR_InUI_Controller.h"
#import "OpenEXR_OutUI_Controller.h"

A_Err
OpenEXR_displayWindowDialog(
	AEIO_BasicData		*basic_dataP,
	const char			*dialogMessage,
	const char			*displayWindowText,
	const char			*dataWindowText,
	A_Boolean			*displayWindow,
	A_Boolean			*displayWindowDefault,
	A_Boolean			*autoShowDialog,
	A_Boolean			*user_interactedPB0)
{
	A_Err			ae_err 		= A_Err_NONE;

#ifdef AE_PROC_INTELx64  // in other words, native Cocoa
	BOOL runAsSubdialog = TRUE;
	NSAutoreleasePool* pool = NULL;
#else
	BOOL runAsSubdialog = FALSE;
	
	NSApplicationLoad();
	NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
#endif

	Class ui_controller_class = [[NSBundle bundleWithIdentifier:@"com.adobe.AfterEffects.OpenEXR"]
									classNamed:@"OpenEXR_displayWindow_Controller"];
	
	if(ui_controller_class)
	{
		OpenEXR_displayWindow_Controller *ui_controller = [[ui_controller_class alloc]
															init: dialogMessage
															dispWtxt: displayWindowText
															dataWtxt: dataWindowText
															displayWindow: *displayWindow
															autoShow: *autoShowDialog
															subDialog: runAsSubdialog];
	
		if(ui_controller)
		{
			NSWindow *my_window = [ui_controller getWindow];
			
			if(my_window)
			{
				NSInteger modal_result = NSRunContinuesResponse;
				DWDialogResult dialog_result = DWDIALOG_RESULT_CONTINUE;

				if(runAsSubdialog)
				{
					// because we're running a modal on top of a modal, we have to do our own							
					// modal loop that we can exit without calling [NSApp endModal], which will also							
					// kill AE's modal dialog.
					NSModalSession modal_session = [NSApp beginModalSessionForWindow:my_window];
					
					do{
						modal_result = [NSApp runModalSession:modal_session];

						dialog_result = [ui_controller getResult];
					}
					while(dialog_result == DWDIALOG_RESULT_CONTINUE && modal_result == NSRunContinuesResponse);
					
					[NSApp endModalSession:modal_session];
				}
				else
				{
					modal_result = [NSApp runModalForWindow:my_window];
				}
				
				
				*displayWindowDefault = [ui_controller getDisplayWindowDefault];
				*autoShowDialog = [ui_controller getAutoShow];

				if(dialog_result == DWDIALOG_RESULT_OK || modal_result == NSRunStoppedResponse)
				{
					*displayWindow = [ui_controller getDisplayWindow];
					
					*user_interactedPB0 = TRUE;
				}
				else
					*user_interactedPB0 = FALSE;
					

				[my_window close];
			}
			
			[ui_controller release];
		}
	}
	
	if(pool)
		[pool release];
	
	return ae_err;
}


A_Err	
OpenEXR_InDialog(
	AEIO_BasicData		*basic_dataP,
	A_Boolean			*cache_channels,
	A_long				*num_caches,
	A_Boolean			*cache_everything,
	A_Boolean			*user_interactedPB0)
{
	A_Err			ae_err 		= A_Err_NONE;

#ifdef AE_PROC_INTELx64  // in other words, native Cocoa
	BOOL runAsSubdialog = TRUE;
	NSAutoreleasePool* pool = NULL;
#else
	BOOL runAsSubdialog = FALSE;
	
	NSApplicationLoad();
	NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
#endif

	Class ui_controller_class = [[NSBundle bundleWithIdentifier:@"com.adobe.AfterEffects.OpenEXR"]
									classNamed:@"OpenEXR_InUI_Controller"];
	
	if(ui_controller_class)
	{
		OpenEXR_InUI_Controller *ui_controller = [[ui_controller_class alloc]
													init:*cache_channels
													numCaches:*num_caches
													cacheEverything:*cache_everything
													subDialog:runAsSubdialog];
		if(ui_controller)
		{
			NSWindow *my_window = [ui_controller getWindow];
			
			if(my_window)
			{
				NSInteger modal_result = NSRunContinuesResponse;
				InDialogResult dialog_result = INDIALOG_RESULT_CONTINUE;

				if(runAsSubdialog)
				{
					// because we're running a modal on top of a modal, we have to do our own							
					// modal loop that we can exit without calling [NSApp endModal], which will also							
					// kill AE's modal dialog.
					NSModalSession modal_session = [NSApp beginModalSessionForWindow:my_window];
					
					do{
						modal_result = [NSApp runModalSession:modal_session];

						dialog_result = [ui_controller getResult];
					}
					while(dialog_result == INDIALOG_RESULT_CONTINUE && modal_result == NSRunContinuesResponse);
					
					[NSApp endModalSession:modal_session];
				}
				else
				{
					modal_result = [NSApp runModalForWindow:my_window];
				}

				if(dialog_result == INDIALOG_RESULT_OK || modal_result == NSRunStoppedResponse)
				{
					*cache_channels = [ui_controller getCache];
					*num_caches = [ui_controller getNumCaches];
					*cache_everything = [ui_controller getCacheEverything];
					
					*user_interactedPB0 = TRUE;
				}
				else
					*user_interactedPB0 = FALSE;
					

				[my_window close];
			}
			
			[ui_controller release];
		}
	}
	
	if(pool)
		[pool release];
	
	return ae_err;
}


A_Err	
OpenEXR_OutDialog(
	AEIO_BasicData		*basic_dataP,
	OpenEXR_outData		*options,
	A_Boolean			*user_interactedPB0)
{
	A_Err			ae_err 		= A_Err_NONE;

#ifdef AE_PROC_INTELx64  // in other words, native Cocoa
	BOOL runAsSubdialog = TRUE;
	NSAutoreleasePool* pool = NULL;
#else
	BOOL runAsSubdialog = FALSE;
	
	NSApplicationLoad();
	NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
#endif
	
	Class ui_controller_class = [[NSBundle bundleWithIdentifier:@"com.adobe.AfterEffects.OpenEXR"]
									classNamed:@"OpenEXR_OutUI_Controller"];
	
	if(ui_controller_class)
	{
		OpenEXR_OutUI_Controller *ui_controller = [[ui_controller_class alloc] init];
		
		if(ui_controller)
		{
			[ui_controller setSubDialog:runAsSubdialog];
		
			[ui_controller setCompression:options->compression_type];
			[ui_controller setLumiChrom:options->luminance_chroma];
			[ui_controller setFloat:options->float_not_half];
			
			NSWindow *my_window = [ui_controller getWindow];
							
			if(my_window)
			{							
				NSInteger modal_result = NSRunContinuesResponse;
				DialogResult dialog_result = DIALOG_RESULT_CONTINUE;

				if(runAsSubdialog)
				{
					// because we're running a modal on top of a modal, we have to do our own							
					// modal loop that we can exit without calling [NSApp endModal], which will also							
					// kill AE's modal dialog.
					NSModalSession modal_session = [NSApp beginModalSessionForWindow:my_window];
					
					do{
						modal_result = [NSApp runModalSession:modal_session];

						dialog_result = [ui_controller getResult];
					}
					while(dialog_result == DIALOG_RESULT_CONTINUE && modal_result == NSRunContinuesResponse);
					
					[NSApp endModalSession:modal_session];
				}
				else
				{
					modal_result = [NSApp runModalForWindow:my_window];
				}

				if(dialog_result == DIALOG_RESULT_OK || modal_result == NSRunStoppedResponse)
				{
					options->compression_type = [ui_controller getCompression];
					options->luminance_chroma = [ui_controller getLumiChrom];
					options->float_not_half = [ui_controller getFloat];
					
					*user_interactedPB0 = TRUE;
				}
				else
					*user_interactedPB0 = FALSE;
					

				[my_window close];
			}
			
			[ui_controller release];
		}
	}
	
	if(pool)
		[pool release];
	
	return ae_err;
}


void
OpenEXR_CopyPluginPath(
	A_char				*pathZ,
	A_long				max_len)
{
	CFBundleRef bundle_ref = CFBundleGetBundleWithIdentifier(CFSTR("com.adobe.AfterEffects.OpenEXR"));
	CFRetain(bundle_ref);
	
	CFURLRef plug_in_url = CFBundleCopyBundleURL(bundle_ref);
	
	Boolean got_path = CFURLGetFileSystemRepresentation(plug_in_url, TRUE, (UInt8 *)pathZ, max_len);
	
	CFRelease(plug_in_url);
	CFRelease(bundle_ref);
}


bool
ShiftKeyHeld()
{
	NSUInteger flags = [[NSApp currentEvent] modifierFlags];
	
	return ( (flags & NSShiftKeyMask) || (flags & NSAlternateKeyMask) );
}
