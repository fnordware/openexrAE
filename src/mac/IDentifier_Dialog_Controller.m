#import "IDentifier_Dialog_Controller.h"

@implementation IDentifier_Dialog_Controller
- (id)init {
	self = [super init];
	
	if(!([NSBundle loadNibNamed:@"IDentifier_Dialog" owner:self]))
		return nil;
	
	[window center];
	
	[idMenu setAutoenablesItems:FALSE];
	
	return self;
}

- (IBAction)clickedOK:(NSButton *)sender {
    [NSApp stopModal];
}

- (IBAction)clickedCancel:(NSButton *)sender {
    [NSApp abortModal];
}

- (NSPopUpButton *)getMenu {
	return idMenu;
}

- (NSWindow *)getWindow {
	return window;
}
@end
