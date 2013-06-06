#import <Cocoa/Cocoa.h>


#if !NSINTEGER_DEFINED
typedef int NSInteger;
#define NSINTEGER_DEFINED 1
#endif

typedef enum {
	DIALOG_RESULT_CONTINUE = 0,
	DIALOG_RESULT_OK,
	DIALOG_RESULT_CANCEL
} DialogResult;

@interface OpenEXR_OutUI_Controller : NSObject {
    IBOutlet NSWindow *theWindow;
    IBOutlet NSPopUpButton *compressionPulldown;
    IBOutlet NSButton *floatCheck;
	IBOutlet NSTextField *floatLabel;
    IBOutlet NSButton *lumiChromCheck;
	BOOL subDialog;
	DialogResult theResult;
}
- (IBAction)trackLumiChrom:(id)sender;
- (IBAction)clickOK:(id)sender;
- (IBAction)clickCancel:(id)sender;
- (NSWindow *)getWindow;
- (void)setSubDialog:(BOOL)subDialogVal;
- (DialogResult)getResult;

- (NSInteger)getCompression;
- (void)setCompression:(NSInteger)compression;
- (BOOL)getLumiChrom;
- (void)setLumiChrom:(BOOL)lumiChrom;
- (BOOL)getFloat;
- (void)setFloat:(BOOL)useFloat;
@end
