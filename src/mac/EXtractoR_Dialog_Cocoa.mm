//
//	EXtractoR
//		by Brendan Bolles <brendan@fnordware.com>
//
//	extract float channels
//
//	Part of the fnord OpenEXR tools
//		http://www.fnordware.com/OpenEXR/
//
//

#include "EXtractoR_Dialog.h"

#import "EXtractoR_Dialog_Controller.h"

bool EXtractoR_Dialog(
	const Imf::ChannelList	&channels,
	const LayerMap		&layers,
	std::string			&red,
	std::string			&green,
	std::string			&blue,
	std::string			&alpha,
	const char			*plugHndl,
	const void			*mwnd)
{
	bool clicked_ok = false;

	NSApplicationLoad();
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

	NSString *bundle_id = [NSString stringWithUTF8String:plugHndl];

	Class ui_controller_class = [[NSBundle bundleWithIdentifier:bundle_id]
									classNamed:@"EXtractoR_Dialog_Controller"];

	if(ui_controller_class)
	{
		// convert C++ containers to Objective-C
		NSMutableArray *ns_channels = [[NSMutableArray alloc] init];
		
		for(Imf::ChannelList::ConstIterator i = channels.begin(); i != channels.end(); ++i)
		{
			[ns_channels addObject:[NSString stringWithUTF8String:i.name()]];
		}
		
		
		NSMutableDictionary *ns_layers = [[NSMutableDictionary alloc] init];
		
		for(LayerMap::const_iterator i = layers.begin(); i != layers.end(); ++i)
		{
			const ChannelVec &layer_channels = i->second;
			
			NSMutableArray *ns_layer_channels = [[NSMutableArray alloc] init];
			
			for(int j = 0; j < layer_channels.size(); j++)
			{
				[ns_layer_channels insertObject:[NSString stringWithUTF8String:layer_channels[j].c_str()] atIndex:j];
			}
			
			[ns_layers setObject:ns_layer_channels forKey:[NSString stringWithUTF8String:i->first.c_str()]];
			
			[ns_layer_channels release];
		}
		
		
		EXtractoR_Dialog_Controller *ui_controller = [[ui_controller_class alloc]
														init:ns_channels
														layers:ns_layers
														red:[NSString stringWithUTF8String:red.c_str()]
														green:[NSString stringWithUTF8String:green.c_str()]
														blue:[NSString stringWithUTF8String:blue.c_str()]
														alpha:[NSString stringWithUTF8String:alpha.c_str()] ];
		if(ui_controller)
		{
			NSWindow *my_window = [ui_controller getWindow];
			
			if(my_window)
			{
				NSInteger result = [NSApp runModalForWindow:my_window];
				
				if(result == NSRunStoppedResponse)
				{
					red = [[ui_controller getRed] UTF8String];
					green = [[ui_controller getGreen] UTF8String];
					blue = [[ui_controller getBlue] UTF8String];
					alpha = [[ui_controller getAlpha] UTF8String];
					
					clicked_ok = true;
				}
				
				[my_window release];
			}
			
			[ui_controller release];
		}
		
		[ns_layers release];
		[ns_channels release];
	}
	
	if(pool)
		[pool release];

	return clicked_ok;
}


void SetMickeyCursor()
{
	[[NSCursor pointingHandCursor] set];
}
