//
//	IDentifier
//		by Brendan Bolles <brendan@fnordware.com>
//
//	extract discrete UINT channels
//
//	Part of the fnord OpenEXR tools
//		http://www.fnordware.com/OpenEXR/
//
//


#include "IDentifier.h"

#import "IDentifier_Dialog_Controller.h"


PF_Err 
DoDialog (
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output )
{
	A_Err			err 		= A_Err_NONE;

	AEGP_SuiteHandler suites(in_data->pica_basicP);
	PF_ChannelSuite *cs = suites.PFChannelSuite();


	A_long chan_count;
	cs->PF_GetLayerChannelCount(in_data->effect_ref, 0, &chan_count);

	if(chan_count == 0)
	{
		PF_SPRINTF(out_data->return_msg, "No auxiliary channels available.");
	}
	else
	{
		ArbitraryData *arb_data = (ArbitraryData *)PF_LOCK_HANDLE(params[EXTRACT_DATA]->u.arb_d.value);
		ChannelStatus *channel_status = (ChannelStatus *)PF_LOCK_HANDLE(in_data->sequence_data);
		
		// reconcile incorrect indices
		if(channel_status->status == STATUS_INDEX_CHANGE)
			arb_data->index = channel_status->index;
		
		// let's do this
		Class ui_controller_class = [[NSBundle bundleWithIdentifier:@"com.adobe.AfterEffects.IDentifier"]
										classNamed:@"IDentifier_Dialog_Controller"];
		
		if(ui_controller_class)
		{
			IDentifier_Dialog_Controller *ui_controller = [[ui_controller_class alloc] init];
			
			if(ui_controller)
			{
				// fill in menu with channels
				int valid_channels = 0;
				
				NSPopUpButton *menu = [ui_controller getMenu];
				
				// we have one menu item to start, and we want to get rid of it
				[menu removeAllItems];
				
				do{
					PF_Err				err 	= PF_Err_NONE;
					
					A_long chan_count;
					cs->PF_GetLayerChannelCount(in_data->effect_ref, 0, &chan_count);
					
					for(int i=0; i<chan_count; i++)
					{
						PF_Boolean	found;

						PF_ChannelRef chan_ref;
						PF_ChannelDesc chan_desc;
						
						// get the channel
						cs->PF_GetLayerChannelIndexedRefAndDesc(in_data->effect_ref,
																0,
																i,
																&found,
																&chan_ref,
																&chan_desc);
																
						// found isn't really what it used to be
						found = (found && chan_desc.channel_type && chan_desc.data_type);
						
						if(found && chan_desc.dimension == 1)
						{
							[menu addItemWithTitle:[NSString stringWithUTF8String:chan_desc.name]];
							
							[[menu lastItem] setTag:i];
							
							if( !(	chan_desc.data_type == PF_DataType_CHAR ||
									chan_desc.data_type == PF_DataType_U_BYTE ||
									chan_desc.data_type == PF_DataType_U_SHORT ||
									chan_desc.data_type == PF_DataType_LONG  ) )
							{
								// sorry, we need 1D unsigned integers
								[[menu lastItem] setEnabled:FALSE];
							}
							else
							{
								valid_channels++;
							}
						}
					}
						
				}while(0);


				if(valid_channels < 1)
				{
					PF_SPRINTF(out_data->return_msg, "No ID channels available.");
				}
				else
				{
					// set menu value
					[menu selectItemAtIndex:arb_data->index];
					
					// but we don't want a disabled menu item selected when the dialog pops up
					if(![[menu selectedItem] isEnabled])
					{
						// select the first enabled item
						for(int i=0; i < [menu numberOfItems]; i++)
						{
							if([[menu itemAtIndex:i] isEnabled])
							{
								[menu selectItemAtIndex:i];
								break;
							}
						}
					}

					NSWindow *my_window = [ui_controller getWindow];
									
					if(my_window)
					{
						NSInteger result = [NSApp runModalForWindow:my_window];
						
						if(result == NSRunStoppedResponse)
						{
							PF_Boolean	found;

							PF_ChannelRef chan_ref;
							PF_ChannelDesc chan_desc;
							
							// get the channel
							cs->PF_GetLayerChannelIndexedRefAndDesc(in_data->effect_ref,
																	0,
																	[[menu selectedItem] tag],
																	&found,
																	&chan_ref,
																	&chan_desc);
																	
							// found isn't really what it used to be
							found = (found && chan_desc.channel_type && chan_desc.data_type);

							if(found)
							{
								arb_data->index = [[menu selectedItem] tag];
								strncpy(arb_data->name, chan_desc.name, MAX_CHANNEL_NAME_LEN);
								channel_status->status = STATUS_NORMAL;
							}
							else
							{
								channel_status->status = STATUS_ERROR;
							}
						
							params[EXTRACT_DATA]->uu.change_flags = PF_ChangeFlag_CHANGED_VALUE;
						}
						
						[my_window release];
					}
				}
					
				[ui_controller release];
			}
		}
		
		PF_UNLOCK_HANDLE(params[EXTRACT_DATA]->u.arb_d.value);
		PF_UNLOCK_HANDLE(in_data->sequence_data);
	}

	return err;
}

void SetMickeyCursor()
{
	[[NSCursor pointingHandCursor] set];
}
