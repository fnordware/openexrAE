//
//	OpenEXR file importer/exporter for After Effects (AEIO)
// 
//	by Brendan Bolles <brendan@fnordware.com>
//
//	see OpenEXR.cpp for more information
//

#import "OpenEXR_InUI_Controller.h"

@implementation OpenEXR_InUI_Controller

- (id)init:(BOOL)cache
	numCaches:(NSInteger)num_cashes
	cacheEverything:(BOOL)cache_everything
	subDialog:(BOOL)sub_dialog
{
	self = [super init];
	
	if(!([NSBundle loadNibNamed:@"OpenEXR_InDialog" owner:self]))
		return nil;
	
	[theWindow center];
	
	[cacheCheck setState:(cache ? NSOnState : NSOffState)];
	[cacheEverythingCheck setState:(cache_everything ? NSOnState : NSOffState)];
	
	// fill in menu, range determined here
	int i;
	for(i=0; i <= 10; i++)
	{
		[numCachesPulldown insertItemWithTitle:[NSString stringWithFormat:@"%d", i] atIndex:i];
		[[numCachesPulldown itemAtIndex:i] setTag:i];
	}
	
	[numCachesPulldown selectItemWithTag:num_cashes];
	
	subDialog = sub_dialog;
	
	theResult = INDIALOG_RESULT_CONTINUE;
	
	return self;
}

- (IBAction)clickOK:(id)sender {
	if(subDialog)
		theResult = INDIALOG_RESULT_OK;
	else
		[NSApp stopModal];
}

- (IBAction)clickCancel:(id)sender {
	if(subDialog)
		theResult = INDIALOG_RESULT_CANCEL;
	else
		[NSApp abortModal];
}

- (InDialogResult)getResult {
	return theResult;
}

- (NSWindow *)getWindow {
	return theWindow;
}

- (BOOL)getCache {
	return ([cacheCheck state] == NSOnState);
}

- (NSInteger)getNumCaches {
	return [[numCachesPulldown selectedItem] tag];
}

- (BOOL)getCacheEverything {
	return ([cacheEverythingCheck state] == NSOnState);
}

@end
