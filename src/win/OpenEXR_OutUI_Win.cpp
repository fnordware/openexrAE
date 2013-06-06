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
	OUT_noUI = -1,
	OUT_OK = IDOK,
	OUT_Cancel = IDCANCEL,
	OUT_Compression_Menu = 3,
	OUT_LumiChrom_Check,
	OUT_Float_Check,
	OUT_Float_NotRecom
};


enum
{
    OUT_NO_COMPRESSION  = 0,	// no compression
    OUT_RLE_COMPRESSION,	// run length encoding
    OUT_ZIPS_COMPRESSION,	// zlib compression, one scan line at a time
    OUT_ZIP_COMPRESSION,	// zlib compression, in blocks of 16 scan lines
    OUT_PIZ_COMPRESSION,	// piz-based wavelet compression
    OUT_PXR24_COMPRESSION,	// lossy 24-bit float compression
	OUT_B44_COMPRESSION,	// lossy 16-bit float compression
	OUT_B44A_COMPRESSION,	// B44 w/ bonus area compression

    OUT_NUM_COMPRESSION_METHODS	// number of different compression methods
};

// globals
HINSTANCE hDllInstance = NULL;

static WORD	g_item_clicked = 0;

static A_u_char		g_Compression	= OUT_PIZ_COMPRESSION;
static A_Boolean	g_lumi_chrom	= FALSE;
static A_Boolean	g_32bit_float	= FALSE;


static void TrackLumiChrom(HWND hwndDlg)
{
	BOOL enable_state = !SendMessage(GetDlgItem(hwndDlg, OUT_LumiChrom_Check), BM_GETCHECK, (WPARAM)0, (LPARAM)0);

	EnableWindow(GetDlgItem(hwndDlg, OUT_Float_Check), enable_state);
	EnableWindow(GetDlgItem(hwndDlg, OUT_Float_NotRecom), enable_state);
}


static BOOL CALLBACK DialogProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam) 
{ 
    BOOL fError; 
 
    switch (message) 
    { 
		case WM_INITDIALOG:
			do{
				// set up the menu
				// I prefer to do it programatically to insure that the compression types match the index
				char *op1 = "None";
				char *op2 = "RLE";
				char *op3 = "Zip";
				char *op4 = "Zip16";
				char *op5 = "Piz";
				char *op6 = "PXR24";
				char *op7 = "B44";
				char *op8 = "B44A";

				char *opts[] = {op1, op2, op3, op4, op5, op6, op7, op8};

				HWND menu = GetDlgItem(hwndDlg, OUT_Compression_Menu);

				int i;

				for(int i=OUT_NO_COMPRESSION; i<=OUT_B44A_COMPRESSION; i++)
				{
					SendMessage(menu,( UINT)CB_ADDSTRING, (WPARAM)wParam, (LPARAM)(LPCTSTR)opts[i] );
					SendMessage( menu,(UINT)CB_SETITEMDATA, (WPARAM)i, (LPARAM)(DWORD)i); // this is the compresion number

					if(i == g_Compression)
						SendMessage( menu, CB_SETCURSEL, (WPARAM)i, (LPARAM)0);
				}
			}while(0);

			SendMessage(GetDlgItem(hwndDlg, OUT_LumiChrom_Check), BM_SETCHECK, (WPARAM)g_lumi_chrom, (LPARAM)0);
			SendMessage(GetDlgItem(hwndDlg, OUT_Float_Check), BM_SETCHECK, (WPARAM)g_32bit_float, (LPARAM)0);

			TrackLumiChrom(hwndDlg);

			return TRUE;
 

        case WM_COMMAND: 
			g_item_clicked = LOWORD(wParam);

            switch (LOWORD(wParam)) 
            { 
                case OUT_OK: 
				case OUT_Cancel:  // do the same thing, but g_item_clicked will be different
					do{
						HWND menu = GetDlgItem(hwndDlg, OUT_Compression_Menu);

						// get the channel index associated with the selected menu item
						LRESULT cur_sel = SendMessage(menu,(UINT)CB_GETCURSEL, (WPARAM)0, (LPARAM)0);

						g_Compression = SendMessage(menu,(UINT)CB_GETITEMDATA, (WPARAM)cur_sel, (LPARAM)0);
						g_lumi_chrom = SendMessage(GetDlgItem(hwndDlg, OUT_LumiChrom_Check), BM_GETCHECK, (WPARAM)0, (LPARAM)0);
						g_32bit_float = SendMessage(GetDlgItem(hwndDlg, OUT_Float_Check), BM_GETCHECK, (WPARAM)0, (LPARAM)0);

					}while(0);

					//PostMessage((HWND)hwndDlg, WM_QUIT, (WPARAM)WA_ACTIVE, lParam);
					EndDialog(hwndDlg, 0);
                    //DestroyWindow(hwndDlg); 
                    return TRUE;

				case OUT_LumiChrom_Check:
					TrackLumiChrom(hwndDlg);
					return TRUE;
            } 
    } 
    return FALSE; 
} 


static PF_Err 
DoWinDialog (AEIO_BasicData		*basic_dataP)
{
	A_Err			err 		= A_Err_NONE;

	AEGP_SuiteHandler	suites(basic_dataP->pica_basicP);

	// get platform handles
	HWND hwndOwner = NULL;
	suites.UtilitySuite()->AEGP_GetMainHWND((void *)&hwndOwner);

	int status = DialogBox(hDllInstance, (LPSTR)"OUTDIALOG", hwndOwner, (DLGPROC)DialogProc);

	return err;
}



A_Err	
OpenEXR_OutDialog(
	AEIO_BasicData		*basic_dataP,
	OpenEXR_outData		*options,
	A_Boolean			*user_interactedPB0)
{
	A_Err			err 		= A_Err_NONE;
	
	// set globals
	g_Compression = options->compression_type;
	g_lumi_chrom = options->luminance_chroma;
	g_32bit_float = options->float_not_half;
	

	// do dialog, passing plug-in path in refcon
	DoWinDialog(basic_dataP);
	

	if(g_item_clicked == OUT_OK)
	{
		options->compression_type = g_Compression;
		options->luminance_chroma = g_lumi_chrom;
		options->float_not_half = g_32bit_float;
		
		*user_interactedPB0 = TRUE;
	}
	else
		*user_interactedPB0 = FALSE;
		
	return err;
}


void
OpenEXR_CopyPluginPath(
	A_char				*pathZ,
	A_long				max_len)
{
	DWORD result = GetModuleFileName(hDllInstance, pathZ, max_len);
}


BOOL WINAPI DllMain(HANDLE hInstance, DWORD fdwReason, LPVOID lpReserved)
{
	if (fdwReason == DLL_PROCESS_ATTACH)
		hDllInstance = (HINSTANCE)hInstance;

	return TRUE;   // Indicate that the DLL was initialized successfully.
}
