#import <Cocoa/Cocoa.h>

@interface IDentifier_Dialog_Controller : NSObject {
    IBOutlet NSPopUpButton *idMenu;
    IBOutlet NSWindow *window;
}
- (IBAction)clickedOK:(NSButton *)sender;
- (IBAction)clickedCancel:(NSButton *)sender;

- (NSPopUpButton *)getMenu;
- (NSWindow *)getWindow;
@end
