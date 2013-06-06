#import "OpenEXR_OutUI_Controller.h"

@implementation OpenEXR_OutUI_Controller
- (id)init {
	self = [super init];
	
	if(!([NSBundle loadNibNamed:@"OpenEXR_Dialog" owner:self]))
		return nil;
	
	[theWindow center];
	
	[compressionPulldown removeAllItems];
	[compressionPulldown addItemsWithTitles:
		[NSArray arrayWithObjects:@"None", @"RLE", @"Zip", @"Zip16", @"Piz", @"PXR24", @"B44", @"B44A", nil]];

	theResult = DIALOG_RESULT_CONTINUE;

	return self;
}

- (IBAction)trackLumiChrom:(id)sender {
	BOOL enabled =  ([lumiChromCheck state] == NSOffState);
	NSColor *label_color = (enabled ? [NSColor textColor] : [NSColor disabledControlTextColor]);
	
    [floatCheck setEnabled:enabled];
	[floatLabel setTextColor:label_color];
	
	[label_color release];
}

- (IBAction)clickOK:(id)sender {
	if(subDialog)
		theResult = DIALOG_RESULT_OK;
	else
		[NSApp stopModal];
}

- (IBAction)clickCancel:(id)sender {
	if(subDialog)
		theResult = DIALOG_RESULT_CANCEL;
	else
		[NSApp abortModal];
}

- (NSWindow *)getWindow {
	return theWindow;
}

- (void)setSubDialog:(BOOL)subDialogVal {
	subDialog = subDialogVal;
}

- (DialogResult)getResult {
	return theResult;
}

- (NSInteger)getCompression {
	return [compressionPulldown indexOfSelectedItem];
}

- (void)setCompression:(NSInteger)compression {
	[compressionPulldown selectItem:[compressionPulldown itemAtIndex:compression]];
}

- (BOOL)getLumiChrom {
	return ([lumiChromCheck state] == NSOnState);
}

- (void)setLumiChrom:(BOOL)lumiChrom {
	[lumiChromCheck setState:(lumiChrom ? NSOnState : NSOffState)];
	[self trackLumiChrom:nil]; // used to track the control when the dialog opens
}

- (BOOL)getFloat {
	return ([floatCheck state] == NSOnState);
}

- (void)setFloat:(BOOL)useFloat {
	[floatCheck setState:(useFloat ? NSOnState : NSOffState)];
}
@end
