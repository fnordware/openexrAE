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
	INDIALOG_RESULT_CONTINUE = 0,
	INDIALOG_RESULT_OK,
	INDIALOG_RESULT_CANCEL
} InDialogResult;

@interface OpenEXR_InUI_Controller : NSObject {
	IBOutlet NSWindow *theWindow;
	IBOutlet NSButton *cacheCheck;
	IBOutlet NSPopUpButton *numCachesPulldown;
	IBOutlet NSButton *cacheEverythingCheck;
	BOOL subDialog;
	InDialogResult theResult;
}

- (id)init:(BOOL)cache
	numCaches:(NSInteger)num_cashes
	cacheEverything:(BOOL)cache_everything
	subDialog:(BOOL)sub_dialog;

- (IBAction)clickOK:(id)sender;
- (IBAction)clickCancel:(id)sender;
- (InDialogResult)getResult;

- (NSWindow *)getWindow;

- (BOOL)getCache;
- (NSInteger)getNumCaches;
- (BOOL)getCacheEverything;

@end
