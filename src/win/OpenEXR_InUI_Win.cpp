//
//	OpenEXR file importer/exported for After Effects (AEIO)
// 
//	by Brendan Bolles <brendan@fnordware.com>
//
//	see OpenEXR.cpp for more information
//

#include "OpenEXR_UI.h"

#include <Windows.h>

#include <stdio.h>

// dialog comtrols
enum {
	IN_noUI = -1,
	IN_OK = IDOK,
	IN_Cancel = IDCANCEL,
	IN_Cache_Check = 3,
	IN_Num_Caches_Menu
};


#define GET_ITEM(ITEM)	GetDlgItem(hwndDlg, (ITEM))

#define SET_CHECK(ITEM, VAL)	SendMessage(GET_ITEM(ITEM), BM_SETCHECK, (WPARAM)(VAL), (LPARAM)0)
#define GET_CHECK(ITEM)			SendMessage(GET_ITEM(ITEM), BM_GETCHECK, (WPARAM)0, (LPARAM)0)

#define ADD_MENU_ITEM(MENU, INDEX, STRING, VALUE, SELECTED) \
				SendMessage(GET_ITEM(MENU),( UINT)CB_ADDSTRING, (WPARAM)wParam, (LPARAM)(LPCTSTR)STRING ); \
				SendMessage(GET_ITEM(MENU),(UINT)CB_SETITEMDATA, (WPARAM)INDEX, (LPARAM)(DWORD)VALUE); \
				if(SELECTED) \
					SendMessage(GET_ITEM(MENU), CB_SETCURSEL, (WPARAM)INDEX, (LPARAM)0);

#define GET_MENU_VALUE(MENU)		SendMessage(GET_ITEM(MENU), (UINT)CB_GETITEMDATA, (WPARAM)SendMessage(GET_ITEM(MENU),(UINT)CB_GETCURSEL, (WPARAM)0, (LPARAM)0), (LPARAM)0)


// globals
extern HINSTANCE hDllInstance;

static WORD	g_item_clicked = 0;

static A_Boolean	g_cache = FALSE;
static A_long		g_num_caches = 3;


static BOOL CALLBACK DialogProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam) 
{ 
    BOOL fError; 
 
    switch (message) 
    { 
		case WM_INITDIALOG:
			do{
				SET_CHECK(IN_Cache_Check, g_cache);

				for(int i=0; i <= 10; i++)
				{
					char str[16];
					sprintf(str, "%d", i);

					ADD_MENU_ITEM(IN_Num_Caches_Menu, i, str, i, (i == g_num_caches));
				}
			}while(0);

			return TRUE;
 

        case WM_COMMAND: 
			g_item_clicked = LOWORD(wParam);

            switch (LOWORD(wParam)) 
            { 
                case IN_OK: 
				case IN_Cancel:  // do the same thing, but g_item_clicked will be different
					do{
						g_cache = GET_CHECK(IN_Cache_Check);

						g_num_caches = GET_MENU_VALUE(IN_Num_Caches_Menu);
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
DoWinDialog(AEIO_BasicData *basic_dataP)
{
	A_Err			err 		= A_Err_NONE;

	AEGP_SuiteHandler suites(basic_dataP->pica_basicP);

	// get parent window from Windows because AEGP_GetMainHWND is buggy in this context
	HWND hwndOwner = GetForegroundWindow();

	if(hwndOwner == NULL)
		suites.UtilitySuite()->AEGP_GetMainHWND((void *)&hwndOwner);

	int status = DialogBox(hDllInstance, (LPSTR)"INDIALOG", hwndOwner, (DLGPROC)DialogProc);

	return err;
}



A_Err	
OpenEXR_InDialog(
	AEIO_BasicData		*basic_dataP,
	A_Boolean			*cache_channels,
	A_long				*num_caches,
	A_Boolean			*user_interactedPB0)
{
	A_Err err = A_Err_NONE;
	
	// set globals
	g_cache = *cache_channels;
	g_num_caches = *num_caches;
	

	// do dialog, passing plug-in path in refcon
	DoWinDialog(basic_dataP);
	

	if(g_item_clicked == IN_OK)
	{
		*cache_channels = g_cache;
		*num_caches = g_num_caches;
		
		*user_interactedPB0 = TRUE;
	}
	else
		*user_interactedPB0 = FALSE;
		
	return err;
}

