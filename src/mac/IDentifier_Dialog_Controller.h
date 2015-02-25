#import <Cocoa/Cocoa.h>

#if !NSINTEGER_DEFINED
typedef int NSInteger;
#define NSINTEGER_DEFINED 1
#endif


@interface IDentifier_Dialog_Controller : NSObject {
    IBOutlet NSPopUpButton *idMenu;
    IBOutlet NSWindow *window;
}
- (IBAction)clickedOK:(NSButton *)sender;
- (IBAction)clickedCancel:(NSButton *)sender;

- (NSPopUpButton *)getMenu;
- (NSWindow *)getWindow;
@end
