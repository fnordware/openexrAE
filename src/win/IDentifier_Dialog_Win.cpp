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

#include "OpenEXR_UTF.h"


// dialog iem IDs
enum {
	DLOG_noUI = -1,
	DLOG_OK = IDOK, // was 1
	DLOG_Cancel = IDCANCEL, // was 2
	DLOG_Channel_Menu = 3
};


// globals
HINSTANCE hDllInstance = NULL;

PF_InData		*g_in_data;
PF_OutData		*g_out_data;

static WORD	g_item_clicked = 0;

static A_long	g_channel = 0;


static inline PF_Boolean ChannelTest(PF_ChannelDesc chan_desc)
{
	return( chan_desc.dimension == 1 &&
			(	chan_desc.data_type == PF_DataType_CHAR ||
				chan_desc.data_type == PF_DataType_U_BYTE ||
				chan_desc.data_type == PF_DataType_U_SHORT ||
				chan_desc.data_type == PF_DataType_LONG  ) );
}


BOOL CALLBACK DialogProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam) 
{ 
    BOOL fError; 
 
    switch (message) 
    { 
      case WM_INITDIALOG:
		  do{
			// add lists to combo boxes
			AEGP_SuiteHandler suites(g_in_data->pica_basicP);
			PF_ChannelSuite *cs = suites.PFChannelSuite();

			A_long chan_count;
			cs->PF_GetLayerChannelCount(g_in_data->effect_ref, 0, &chan_count);

			HWND menu = GetDlgItem(hwndDlg, DLOG_Channel_Menu);
			
			int current_index = 0;

			int i;

			// add the compatible channels
			for(i=0; i < chan_count; i++)
			{
				PF_Boolean	found;

				PF_ChannelRef chan_ref;
				PF_ChannelDesc chan_desc;
				
				// get the channel
				cs->PF_GetLayerChannelIndexedRefAndDesc(g_in_data->effect_ref,
														0,
														i,
														&found,
														&chan_ref,
														&chan_desc);
														
				// found isn't really what it used to be
				found = (found && chan_desc.channel_type && chan_desc.data_type);

				if( found && ChannelTest(chan_desc) )
				{
					WCHAR u_name[PF_CHANNEL_NAME_LEN+1];
					UTF8toUTF16(chan_desc.name, (utf16_char *)u_name, PF_CHANNEL_NAME_LEN+1);

					SendMessage(menu, (UINT)CB_ADDSTRING, (WPARAM)wParam,(LPARAM)(LPCTSTR)u_name );
					SendMessage(menu, (UINT)CB_SETITEMDATA, (WPARAM)current_index, (LPARAM)(DWORD)i); // this is the channel index number

					int item_width = strlen(chan_desc.name) * 7;
					int menu_width = SendMessage(menu, (UINT)CB_GETDROPPEDWIDTH, (WPARAM)0, (LPARAM)0);

					if(item_width > menu_width)
						SendMessage(menu, (UINT)CB_SETDROPPEDWIDTH, (WPARAM)item_width, (LPARAM)0);

					if( g_channel == i )
						SendMessage(menu, CB_SETCURSEL, (WPARAM)current_index, (LPARAM)0);

					current_index++;
				}
			}
		  }while(0);
		return TRUE;
 
        case WM_COMMAND: 
			g_item_clicked = LOWORD(wParam);

            switch (LOWORD(wParam)) 
            { 
                case DLOG_OK: 
				case DLOG_Cancel:  // do the same thing, but g_item_clicked will be different
					do{
						HWND menu = GetDlgItem(hwndDlg, DLOG_Channel_Menu);

						// get the channel index associated with the selected menu item
						LRESULT cur_sel = SendMessage(menu,(UINT)CB_GETCURSEL, (WPARAM)0, (LPARAM)0);

						g_channel = SendMessage(menu,(UINT)CB_GETITEMDATA, (WPARAM)cur_sel, (LPARAM)0);

					}while(0);

					//PostMessage((HWND)hwndDlg, WM_QUIT, (WPARAM)WA_ACTIVE, lParam);
					EndDialog(hwndDlg, 0);
                    //DestroyWindow(hwndDlg); 
                    return TRUE; 
            } 
    } 
    return FALSE; 
} 

static PF_Err 
DoWinDialog (
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output )
{
	A_Err			err 		= A_Err_NONE;

	// init globals
	g_in_data = in_data;
	g_out_data = out_data;


	HWND hwndOwner = NULL;
	PF_GET_PLATFORM_DATA(PF_PlatData_MAIN_WND, (void **)&hwndOwner);

	int status = DialogBox(hDllInstance, TEXT("CHANDIALOG"), hwndOwner, (DLGPROC)DialogProc);

	if(status == -1)
		PF_SPRINTF(	out_data->return_msg, "Dialog error.");

	return err;
}


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
		PF_SPRINTF(	out_data->return_msg, "No auxiliary channels available.");
	}
	else
	{
		// look to see if we have at least one channel that this plug-in can use
		// because we can't disable menu items on Windows, so we just won't show disabled items
		// we could just force (copy) only, but we'll do this to provide more information
		PF_Boolean found_good_channel = FALSE;

		for(int channel_num=0; channel_num < chan_count && !found_good_channel; channel_num++)
		{
			PF_Boolean	found;

			PF_ChannelRef chan_ref;
			PF_ChannelDesc chan_desc;
			
			// get the channel
			cs->PF_GetLayerChannelIndexedRefAndDesc(in_data->effect_ref,
													0,
													channel_num,
													&found,
													&chan_ref,
													&chan_desc);
			
			found_good_channel = ChannelTest(chan_desc);
		}


		if(found_good_channel)
		{
			ArbitraryData *arb_data = (ArbitraryData *)PF_LOCK_HANDLE(params[EXTRACT_DATA]->u.arb_d.value);
			ChannelStatus *channel_status = (ChannelStatus *)PF_LOCK_HANDLE(in_data->sequence_data);
			
			// reconcile incorrect indices
			if(channel_status->status == STATUS_INDEX_CHANGE)
				arb_data->index = channel_status->index;
			

			// init globals
			g_channel = arb_data->index;


			DoWinDialog(in_data, out_data, params, output);


			if(g_item_clicked == DLOG_OK)
			{
				PF_Boolean	found;

				PF_ChannelRef chan_ref;
				PF_ChannelDesc chan_desc;
				
				// get the channel
				cs->PF_GetLayerChannelIndexedRefAndDesc(in_data->effect_ref,
														0,
														g_channel,
														&found,
														&chan_ref,
														&chan_desc);
														
				// found isn't really what it used to be
				found = (found && chan_desc.channel_type && chan_desc.data_type);

				if(found)
				{
					arb_data->index = g_channel;
					strncpy(arb_data->name, chan_desc.name, MAX_CHANNEL_NAME_LEN);
					channel_status->status = STATUS_NORMAL;
				}
				else
				{
					channel_status->status = STATUS_ERROR;
				}
			
				params[EXTRACT_DATA]->uu.change_flags = PF_ChangeFlag_CHANGED_VALUE;
			}

			PF_UNLOCK_HANDLE(params[EXTRACT_DATA]->u.arb_d.value);
			PF_UNLOCK_HANDLE(in_data->sequence_data);
		}
		else
			PF_SPRINTF(	out_data->return_msg, "No suitable integer channels available.");

	}

	return err;
}

BOOL WINAPI DllMain(HANDLE hInstance, DWORD fdwReason, LPVOID lpReserved)
{
	if (fdwReason == DLL_PROCESS_ATTACH)
		hDllInstance = (HINSTANCE)hInstance;

	return TRUE;   // Indicate that the DLL was initialized successfully.
}