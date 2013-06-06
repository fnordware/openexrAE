//
//	OpenEXR file importer/exported for After Effects (AEIO)
// 
//	by Brendan Bolles <brendan@fnordware.com>
//
//	see OpenEXR.cpp for more information
//

#include "OpenEXR_UI.h"

#include <Windows.h>

// dialog comtrols
enum {
	DW_noUI = -1,
	DW_OK = IDOK,
	DW_Cancel = IDCANCEL,
	DW_SetDefault = 3,
	DW_Message_Text,
	DW_displayWindow_Radio,
	DW_dataWindow_Radio,
	DW_autoDisplay_Check
};


#define GET_ITEM(ITEM)	GetDlgItem(hwndDlg, (ITEM))

#define SET_CHECK(ITEM, VAL)	SendMessage(GET_ITEM(ITEM), BM_SETCHECK, (WPARAM)(VAL), (LPARAM)0)
#define GET_CHECK(ITEM)			SendMessage(GET_ITEM(ITEM), BM_GETCHECK, (WPARAM)0, (LPARAM)0)


// globals
extern HINSTANCE hDllInstance;

static WORD	g_item_clicked = 0;

static const char			*g_dialogMessage;
static const char			*g_displayWindowText;
static const char			*g_dataWindowText;
static A_Boolean			g_displayWindow;
static A_Boolean			g_displayWindowDefault;
static A_Boolean			g_autoShowDialog;


static BOOL CALLBACK DialogProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam) 
{ 
    BOOL fError; 
 
    switch (message) 
    { 
		case WM_INITDIALOG:
			do{
				SetDlgItemText(hwndDlg, DW_Message_Text, g_dialogMessage);
				SetDlgItemText(hwndDlg, DW_displayWindow_Radio, g_displayWindowText);
				SetDlgItemText(hwndDlg, DW_dataWindow_Radio, g_dataWindowText);

				CheckRadioButton(hwndDlg, DW_displayWindow_Radio, DW_dataWindow_Radio,
								(g_displayWindow ? DW_displayWindow_Radio : DW_dataWindow_Radio));

				SET_CHECK(DW_autoDisplay_Check, g_autoShowDialog);
			}while(0);

			return TRUE;
 

        case WM_COMMAND: 
			g_item_clicked = LOWORD(wParam);

            switch (LOWORD(wParam)) 
            { 
                case DW_OK: 
				case DW_Cancel:  // do the same thing, but g_item_clicked will be different
					do{
						g_displayWindow = IsDlgButtonChecked(hwndDlg, DW_displayWindow_Radio);
						g_autoShowDialog = GET_CHECK(DW_autoDisplay_Check);
					}while(0);

					//PostMessage((HWND)hwndDlg, WM_QUIT, (WPARAM)WA_ACTIVE, lParam);
					EndDialog(hwndDlg, 0);
                    //DestroyWindow(hwndDlg); 
                    return TRUE;

				case DW_SetDefault:
					g_displayWindowDefault = IsDlgButtonChecked(hwndDlg, DW_displayWindow_Radio);
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

	int status = DialogBox(hDllInstance, (LPSTR)"DWDIALOG", hwndOwner, (DLGPROC)DialogProc);

	return err;
}


A_Err
OpenEXR_displayWindowDialog(
	AEIO_BasicData		*basic_dataP,
	const char			*dialogMessage,
	const char			*displayWindowText,
	const char			*dataWindowText,
	A_Boolean			*displayWindow,
	A_Boolean			*displayWindowDefault,
	A_Boolean			*autoShowDialog,
	A_Boolean			*user_interactedPB0)
{
	A_Err err = A_Err_NONE;
	
	// set globals
	g_dialogMessage = dialogMessage;
	g_displayWindowText = displayWindowText;
	g_dataWindowText = dataWindowText;
	g_displayWindow = *displayWindow;
	g_displayWindowDefault = *displayWindowDefault;
	g_autoShowDialog = *autoShowDialog;
	

	// do dialog, passing plug-in path in refcon
	DoWinDialog(basic_dataP);
	

	*autoShowDialog = g_autoShowDialog;
	*displayWindowDefault = g_displayWindowDefault;

	if(g_item_clicked == DW_OK)
	{
		*displayWindow = g_displayWindow;
		
		*user_interactedPB0 = TRUE;
	}
	else
		*user_interactedPB0 = FALSE;
		
	return err;
}


static inline bool KeyIsDown(int vKey)
{
	return (GetAsyncKeyState(vKey) & 0x8000);
}

bool
ShiftKeyHeld()
{
	return ( KeyIsDown(VK_LSHIFT) || KeyIsDown(VK_RSHIFT) || KeyIsDown(VK_LMENU) || KeyIsDown(VK_RMENU) );
}
