//
//	OpenEXR file importer/exporter for After Effects (AEIO)
// 
//	by Brendan Bolles <brendan@fnordware.com>
//
//	see OpenEXR.cpp for more information
//

#import "OpenEXR_displayWindow_Controller.h"

@implementation OpenEXR_displayWindow_Controller

- (id)init:(const char *)message
	dispWtxt:(const char *)dispW_message
	dataWtxt:(const char *)dataW_message
	displayWindow:(BOOL)dispW
	autoShow:(BOOL)automaticShow
	subDialog:(BOOL)subD
{
	self = [super init];
	
	if(!([NSBundle loadNibNamed:@"OpenEXR_displayWindow" owner:self]))
		return nil;
	
	[theWindow center];
	
	[messageText setStringValue:[NSString stringWithUTF8String:message]];
	[[displayWindowRadioGroup cellAtRow:0 column:0] setTitle:[NSString stringWithUTF8String:dispW_message]];
	[[displayWindowRadioGroup cellAtRow:1 column:0] setTitle:[NSString stringWithUTF8String:dataW_message]];
	
	[displayWindowRadioGroup selectCellAtRow:(dispW ? 0 : 1) column:0];
	
	[autoShowCheck setState:(automaticShow ? NSOnState : NSOffState)];
	
	subDialog = subD;
	
	theResult = DWDIALOG_RESULT_CONTINUE;
	
	displayWindowDefault = dispW;
	
	return self;
}

- (IBAction)clickOK:(id)sender {
	if(subDialog)
		theResult = DWDIALOG_RESULT_OK;
	else
		[NSApp stopModal];
}

- (IBAction)clickCancel:(id)sender {
	if(subDialog)
		theResult = DWDIALOG_RESULT_CANCEL;
	else
		[NSApp abortModal];
}

- (NSWindow *)getWindow {
	return theWindow;
}

- (IBAction)clickSetDefault:(id)sender {
	displayWindowDefault = [self getDisplayWindow];
}

- (DWDialogResult)getResult {
	return theResult;
}

- (BOOL)getDisplayWindow {
	return ([displayWindowRadioGroup selectedRow] == 0);
}

- (BOOL)getDisplayWindowDefault {
	return displayWindowDefault;
}

- (BOOL)getAutoShow {
	return ([autoShowCheck state] == NSOnState);
}

@end
