#import "EXtractoR_Dialog_Controller.h"

@implementation EXtractoR_Dialog_Controller
- (id)init:(NSArray *)chans
	layers:(NSDictionary *)lays
	red:(NSString *)r
	green:(NSString *)g
	blue:(NSString *)b
	alpha:(NSString *)a
{
	self = [super init];
	
	if(!([NSBundle loadNibNamed:@"EXtractoR_Dialog" owner:self]))
		return nil;
	
	[window center];
	
	NSPopUpButton *menus[] = { redMenu, greenMenu, blueMenu, alphaMenu };
	NSString *colors[] = { r, g, b, a };
	
	int i;
	
	for(i=0; i < 4; i++)
	{
		[menus[i] removeAllItems];
		[menus[i] addItemWithTitle:@"(copy)"];
		[menus[i] addItemsWithTitles:chans];
		[menus[i] selectItemWithTitle:colors[i]];
	}
	
	if([lays count] > 0)
	{
		NSEnumerator *layer_enum = [lays keyEnumerator];
		NSString *layer;
		
		while(layer = [layer_enum nextObject])
		{
			[layerMenu addItemWithTitle:layer];
		}
	}
	else
	{
		[layerMenu setEnabled:FALSE];
	}
	
	layers = lays;
	
	
	return self;
}

- (IBAction)clickOK:(NSButton *)sender {
    [NSApp stopModal];
}

- (IBAction)clickCancel:(NSButton *)sender {
    [NSApp abortModal];
}

- (IBAction)trackLayerMenu:(id)sender {
	NSString *layer_name = [[layerMenu selectedItem] title];
	
	NSArray *layer_array = [layers objectForKey:layer_name];
	
	NSPopUpButton *menus[] = { redMenu, greenMenu, blueMenu, alphaMenu };
	
	if(layer_array)
	{
		int i;
		
		if([layer_array count] == 1)
		{
			for(i=0; i < 3; i++)
			{
				[menus[i] selectItemWithTitle:[layer_array objectAtIndex:0]];
			}
			
			[menus[3] selectItemWithTitle:@"(copy)"];
		}
		else
		{
			for(i=0; i < 4; i++)
			{
				if(i < [layer_array count])
				{
					[menus[i] selectItemWithTitle:[layer_array objectAtIndex:i]];
				}
				else
					[menus[i] selectItemWithTitle:@"(copy)"];
			}
		}
	}
	else
	{
		[menus[0] selectItemWithTitle:@"R"];
		[menus[1] selectItemWithTitle:@"G"];
		[menus[2] selectItemWithTitle:@"B"];
		[menus[3] selectItemWithTitle:@"(copy)"];
	}
}

- (NSWindow *)getWindow {
	return window;
}

- (NSString *)getRed {
	[[redMenu selectedItem] title];
}

- (NSString *)getGreen {
	[[greenMenu selectedItem] title];
}

- (NSString *)getBlue {
	[[blueMenu selectedItem] title];
}

- (NSString *)getAlpha {
	[[alphaMenu selectedItem] title];
}

@end
