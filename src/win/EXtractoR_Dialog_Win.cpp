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
//	Windows dialog for EXtractoR

#include "EXtractoR_Dialog.h"

#include "OpenEXR_UTF.h"


// dialog item IDs
enum {
	DLOG_noUI = -1,
	DLOG_OK = IDOK,
	DLOG_Cancel = IDCANCEL,
	DLOG_Red_Menu = 3,
	DLOG_Green_Menu, 
	DLOG_Blue_Menu,
	DLOG_Alpha_Menu,
	DLOG_Layer_Menu
};

// globals
HINSTANCE hDllInstance = NULL;

static const Imf::ChannelList	*g_channels = NULL;
static const LayerMap			*g_layers = NULL;

static WORD	g_item_clicked = 0;

static std::string *g_red = NULL;
static std::string *g_green = NULL;
static std::string *g_blue = NULL;
static std::string *g_alpha = NULL;


static inline PF_Boolean ChannelTest(PF_ChannelDesc chan_desc)
{
	return (chan_desc.data_type == PF_DataType_FLOAT && chan_desc.dimension == 1);
}

BOOL CALLBACK DialogProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam) 
{ 
    BOOL fError; 
 
    switch (message) 
    { 
      case WM_INITDIALOG:
		  do{
			HWND menu[4] = {	GetDlgItem(hwndDlg, DLOG_Red_Menu),
								GetDlgItem(hwndDlg, DLOG_Green_Menu),
								GetDlgItem(hwndDlg, DLOG_Blue_Menu),
								GetDlgItem(hwndDlg, DLOG_Alpha_Menu) };

			std::string *g_value[4] = {	g_red,
										g_green,
										g_blue,
										g_alpha };
			
			int current_index = 0;

			//int i, j;

			// add the (copy) item to each menu
			for(int i=0; i < 4; i++)
			{
				SendMessage( menu[i],(UINT)CB_ADDSTRING,(WPARAM)wParam,(LPARAM)(LPCTSTR)TEXT("(copy)") );
				SendMessage( menu[i],(UINT)CB_SETITEMDATA, (WPARAM)wParam, (LPARAM)(DWORD)0); // (copy) is index 0

				if( *g_value[i] == "(copy)" )
					SendMessage( menu[i], CB_SETCURSEL, (WPARAM)current_index, (LPARAM)0);
			}

			current_index++;

			// add the channels
			for(Imf::ChannelList::ConstIterator i = g_channels->begin(); i != g_channels->end(); ++i)
			{
				for(int j=0; j < 4; j++)
				{
					WCHAR u_name[PF_CHANNEL_NAME_LEN+1];
					UTF8toUTF16(i.name(), (utf16_char *)u_name, PF_CHANNEL_NAME_LEN+1);

					SendMessage(menu[j], (UINT)CB_ADDSTRING, (WPARAM)wParam, (LPARAM)(LPCTSTR)u_name );

					int item_width = strlen(i.name()) * 7;
					int menu_width = SendMessage(menu[j], (UINT)CB_GETDROPPEDWIDTH, (WPARAM)0, (LPARAM)0);

					if(item_width > menu_width)
						SendMessage(menu[j], (UINT)CB_SETDROPPEDWIDTH, (WPARAM)item_width, (LPARAM)0);

					if(*g_value[j] == i.name())
						SendMessage(menu[j], CB_SETCURSEL, (WPARAM)current_index, (LPARAM)0);
				}

				current_index++;
			}


			// add the layers
			HWND layer_menu = GetDlgItem(hwndDlg, DLOG_Layer_Menu);

			if(g_layers->begin() == g_layers->end()) // g_layers->count() isn't working for some reason
			{
				EnableWindow(layer_menu, FALSE);
			}
			else
			{
				for(LayerMap::const_iterator i = g_layers->begin(); i != g_layers->end(); ++i)
				{
					WCHAR u_name[PF_CHANNEL_NAME_LEN+1];
					UTF8toUTF16(i->first, (utf16_char *)u_name, PF_CHANNEL_NAME_LEN+1);

					SendMessage(layer_menu, (UINT)CB_ADDSTRING, (WPARAM)wParam, (LPARAM)(LPCTSTR)u_name );
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
						HWND menu[4] = {	GetDlgItem(hwndDlg, DLOG_Red_Menu),
											GetDlgItem(hwndDlg, DLOG_Green_Menu),
											GetDlgItem(hwndDlg, DLOG_Blue_Menu),
											GetDlgItem(hwndDlg, DLOG_Alpha_Menu) };

						std::string *g_value[4] = {	g_red,
													g_green,
													g_blue,
													g_alpha };

						// get the text for the selected item and store it
						for(int i=0; i < 4; i++)
						{
							LRESULT cur_sel = SendMessage( menu[i],(UINT)CB_GETCURSEL, (WPARAM)0, (LPARAM)0);

							WCHAR u_name[PF_CHANNEL_NAME_LEN+1];
							SendMessage( menu[i],(UINT)CB_GETLBTEXT, (WPARAM)cur_sel, (LPARAM)u_name);

							*g_value[i] = UTF16toUTF8((utf16_char *)u_name);
						}

					}while(0);

					//PostMessage((HWND)hwndDlg, WM_QUIT, (WPARAM)WA_ACTIVE, lParam);
					EndDialog(hwndDlg, 0);
                    //DestroyWindow(hwndDlg); 
                    return TRUE;


				case DLOG_Layer_Menu:
					if(HIWORD(wParam) == CBN_SELCHANGE)
					{
						HWND layer_menu = GetDlgItem(hwndDlg, DLOG_Layer_Menu);

						LRESULT cur_sel = SendMessage(layer_menu, (UINT)CB_GETCURSEL, (WPARAM)0, (LPARAM)0);

						WCHAR u_name[PF_CHANNEL_NAME_LEN+1];
						SendMessage(layer_menu, (UINT)CB_GETLBTEXT, (WPARAM)cur_sel, (LPARAM)u_name);

						const std::string layer = UTF16toUTF8((utf16_char *)u_name);

						const LayerMap &layers = *g_layers;
						//const ChannelVec &chans = layers[layer]; // this isn't working for some reason....I think it's a visual studio bug!

						ChannelVec channels;

						for(LayerMap::const_iterator i = layers.begin(); i != layers.end(); ++i)
						{
							if(i->first == layer)
							{
								channels = i->second;
							}
						}

						HWND menu[4] = {	GetDlgItem(hwndDlg, DLOG_Red_Menu),
											GetDlgItem(hwndDlg, DLOG_Green_Menu),
											GetDlgItem(hwndDlg, DLOG_Blue_Menu),
											GetDlgItem(hwndDlg, DLOG_Alpha_Menu) };

						if(channels.size() == 1)
						{
							UTF8toUTF16(channels[0], (utf16_char *)u_name, PF_CHANNEL_NAME_LEN+1);

							LRESULT channel_idx = SendMessage(layer_menu, (UINT)CB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)u_name);

							for(int i=0; i < 3; i++)
								SendMessage(menu[i], (UINT)CB_SETCURSEL, (WPARAM)channel_idx, (LPARAM)0);

							SendMessage(menu[3], (UINT)CB_SETCURSEL, (WPARAM)0, (LPARAM)0);
						}
						else
						{
							for(int i=0; i < 4; i++)
							{
								if(i < channels.size())
								{
									UTF8toUTF16(channels[i], (utf16_char *)u_name, PF_CHANNEL_NAME_LEN+1);

									LRESULT channel_idx = SendMessage(menu[i], (UINT)CB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)u_name);

									SendMessage(menu[i], (UINT)CB_SETCURSEL, (WPARAM)channel_idx, (LPARAM)0);
								}
								else
								{
									SendMessage(menu[i], (UINT)CB_SETCURSEL, (WPARAM)0, (LPARAM)0);
								}
							}
						}

						return TRUE;
					}

				return FALSE;
            } 
    } 
    return FALSE; 
} 


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

	g_channels = &channels;
	g_layers = &layers;

	g_red = &red;
	g_green = &green;
	g_blue = &blue;
	g_alpha = &alpha;


	DialogBox(hDllInstance, TEXT("CHANDIALOG"), (HWND)mwnd, (DLGPROC)DialogProc);


	if(g_item_clicked == DLOG_OK)
	{
		// strings already set
		clicked_ok = true;
	}

	return clicked_ok;
}


BOOL WINAPI DllMain(HANDLE hInstance, DWORD fdwReason, LPVOID lpReserved)
{
	if (fdwReason == DLL_PROCESS_ATTACH)
		hDllInstance = (HINSTANCE)hInstance;

	return TRUE;   // Indicate that the DLL was initialized successfully.
}