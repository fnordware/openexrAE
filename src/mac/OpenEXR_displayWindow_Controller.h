//
//	OpenEXR file importer/exporter for After Effects (AEIO)
// 
//	by Brendan Bolles <brendan@fnordware.com>
//
//	see OpenEXR.cpp for more information
//

#import <Cocoa/Cocoa.h>

#if !NSINTEGER_DEFINED
typedef int NSInteger;
typedef unsigned long NSUInteger;
#define NSINTEGER_DEFINED 1
#endif

typedef enum {
	DWDIALOG_RESULT_CONTINUE = 0,
	DWDIALOG_RESULT_OK,
	DWDIALOG_RESULT_CANCEL
} DWDialogResult;

@interface OpenEXR_displayWindow_Controller : NSObject {
	IBOutlet NSWindow *theWindow;
	IBOutlet NSControl *messageText;
	IBOutlet NSMatrix *displayWindowRadioGroup;
	IBOutlet NSButton *autoShowCheck;
	
	BOOL subDialog;
	DWDialogResult theResult;
	
	BOOL displayWindowDefault;
}

- (id)init:(const char *)message
	dispWtxt:(const char *)dispW_message
	dataWtxt:(const char *)dataW_message
	displayWindow:(BOOL)dispW
	autoShow:(BOOL)automaticShow
	subDialog:(BOOL)subD;

- (IBAction)clickOK:(id)sender;
- (IBAction)clickCancel:(id)sender;
- (DWDialogResult)getResult;

- (NSWindow *)getWindow;

- (IBAction)clickSetDefault:(id)sender;

- (BOOL)getDisplayWindow;
- (BOOL)getDisplayWindowDefault;
- (BOOL)getAutoShow;

@end
