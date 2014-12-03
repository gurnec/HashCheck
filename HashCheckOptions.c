/**
 * HashCheck Shell Extension
 * Copyright (C) Kai Liu.  All rights reserved.
 *
 * Please refer to readme.txt for information about this source code.
 * Please refer to license.txt for details about distribution and modification.
 **/

#include "globals.h"
#include "HashCheckCommon.h"
#include "HashCheckOptions.h"
#include "RegHelpers.h"
#include "libs\IsFontAvailable.h"

#define OPTIONS_KEYNAME TEXT("Software\\HashCheck")

typedef struct {
	PHASHCHECKOPTIONS popt;
	HFONT hFont;
	BOOL bFontChanged;
} OPTIONSCONTEXT, *POPTIONSCONTEXT;

static const LOGFONT DEFAULT_FIXED_FONT =
{
	-15,                        // lfHeight (expressed as -2 * PointSize)
	0,                          // lfWidth
	0,                          // lfEscapement
	0,                          // lfOrientation
	FW_REGULAR,                 // lfWeight
	FALSE,                      // lfItalic
	FALSE,                      // lfUnderline
	FALSE,                      // lfStrikeOut
	DEFAULT_CHARSET,            // lfCharSet
	OUT_DEFAULT_PRECIS,         // lfOutPrecision
	CLIP_DEFAULT_PRECIS,        // lfClipPrecision
	DEFAULT_QUALITY,            // lfQuality
	FIXED_PITCH | FF_DONTCARE,  // lfPitchAndFamily
	TEXT("Lucida Console")      // lfFaceName[LF_FACESIZE]
};



/*============================================================================*\
	Function declarations
\*============================================================================*/

// Dialog general
INT_PTR CALLBACK OptionsDlgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
VOID WINAPI OptionsDlgInit( HWND hWnd, POPTIONSCONTEXT poptctx );

// Dialog commands
VOID WINAPI ChangeFont( HWND hWnd, POPTIONSCONTEXT poptctx );
VOID WINAPI SaveSettings( HWND hWnd, POPTIONSCONTEXT poptctx );



/*============================================================================*\
	Public functions
\*============================================================================*/

VOID CALLBACK ShowOptions_RunDLLW( HWND hWnd, HINSTANCE hInstance,
                                   PWSTR pszCmdLine, INT nCmdShow )
{
	HASHCHECKOPTIONS opt;
	OptionsDialog(hWnd, &opt);
}

VOID __fastcall OptionsDialog( HWND hWndOwner, PHASHCHECKOPTIONS popt )
{
	// First, activate our manifest
	ULONG_PTR uActCtxCookie = ActivateManifest(TRUE);

	// Initialize the dialog's context
	OPTIONSCONTEXT optctx = { popt, NULL, FALSE };

	// Show the options dialog; returning the results to our caller is handled
	// by the flags set in the caller's popt
	DialogBoxParam(
		g_hModThisDll,
		MAKEINTRESOURCE(IDD_OPTIONS),
		hWndOwner,
		OptionsDlgProc,
		(LPARAM)&optctx
	);

	// Release the font handle, if the dialog created one
	if (optctx.hFont) DeleteObject(optctx.hFont);

	// Clean up the manifest activation
	DeactivateManifest(uActCtxCookie);
}

VOID __fastcall OptionsLoad( PHASHCHECKOPTIONS popt )
{
	HKEY hKey;

	if (RegOpenKeyEx(HKEY_CURRENT_USER, OPTIONS_KEYNAME, 0, KEY_QUERY_VALUE, &hKey) != ERROR_SUCCESS)
		hKey = NULL;

	if (popt->dwFlags & HCOF_FILTERINDEX)
	{
		if (!( hKey &&
		       RegGetDW(hKey, TEXT("FilterIndex"), &popt->dwFilterIndex) &&
		       popt->dwFilterIndex && popt->dwFilterIndex <= 4 ))
		{
			// Fall back to default (MD5)
			popt->dwFilterIndex = 3;
		}
	}

	if (popt->dwFlags & HCOF_MENUDISPLAY)
	{
		if (!( hKey &&
		       RegGetDW(hKey, TEXT("MenuDisplay"), &popt->dwMenuDisplay) &&
		       popt->dwMenuDisplay < 3 ))
		{
			// Fall back to default (always show)
			popt->dwMenuDisplay = 0;
		}
	}

	if (popt->dwFlags & HCOF_SAVEENCODING)
	{
		if (!( hKey &&
		       RegGetDW(hKey, TEXT("SaveEncoding"), &popt->dwSaveEncoding) &&
		       popt->dwSaveEncoding < 3 ))
		{
			// Fall back to default (UTF-8)
			popt->dwSaveEncoding = 0;
		}
	}

	if (popt->dwFlags & HCOF_FONT)
	{
		DWORD dwType;
		DWORD cbData = sizeof(LOGFONT);

		if (!( hKey && ERROR_SUCCESS ==
		       RegQueryValueEx(hKey, TEXT("Font"), NULL, &dwType, (PBYTE)&popt->lfFont, &cbData) &&
		       dwType == REG_BINARY && cbData == sizeof(LOGFONT)) )
		{
			HDC hDC;

			// Copy the default font (Lucida Console)
			memcpy(&popt->lfFont, &DEFAULT_FIXED_FONT, sizeof(LOGFONT));

			// Use Consolas if it is available
			if (IsFontAvailable(TEXT("Consolas")))
			{
				popt->lfFont.lfHeight = -16;
				popt->lfFont.lfQuality = CLEARTYPE_QUALITY;
				SSStaticCpy(popt->lfFont.lfFaceName, TEXT("Consolas"));
			}

			// Convert our -2 * PointSize to device units
			hDC = GetDC(NULL);
			popt->lfFont.lfHeight = MulDiv(popt->lfFont.lfHeight, GetDeviceCaps(hDC, LOGPIXELSY), 144);
			ReleaseDC(NULL, hDC);
		}
	}

	if (hKey)
		RegCloseKey(hKey);
}

VOID __fastcall OptionsSave( PHASHCHECKOPTIONS popt )
{
	HKEY hKey;

	// Avoid creating the key if nothing is being set
	if (popt->dwFlags == 0)
		return;

	if (hKey = RegOpen(HKEY_CURRENT_USER, OPTIONS_KEYNAME, NULL))
	{
		if (popt->dwFlags & HCOF_FILTERINDEX)
			RegSetDW(hKey, TEXT("FilterIndex"), popt->dwFilterIndex);

		if (popt->dwFlags & HCOF_MENUDISPLAY)
			RegSetDW(hKey, TEXT("MenuDisplay"), popt->dwMenuDisplay);

		if (popt->dwFlags & HCOF_SAVEENCODING)
			RegSetDW(hKey, TEXT("SaveEncoding"), popt->dwSaveEncoding);

		if (popt->dwFlags & HCOF_FONT)
			RegSetValueEx(hKey, TEXT("Font"), 0, REG_BINARY, (PBYTE)&popt->lfFont, sizeof(LOGFONT));

		RegCloseKey(hKey);
	}
}



/*============================================================================*\
	Dialog general
\*============================================================================*/

INT_PTR CALLBACK OptionsDlgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	switch (uMsg)
	{
		case WM_INITDIALOG:
		{
			SetWindowLongPtr(hWnd, DWLP_USER, lParam);
			OptionsDlgInit(hWnd, (POPTIONSCONTEXT)lParam);
			return(TRUE);
		}

		case WM_ENDSESSION:
		case WM_CLOSE:
		{
			goto end_dialog;
		}

		case WM_COMMAND:
		{
			POPTIONSCONTEXT poptctx = (POPTIONSCONTEXT)GetWindowLongPtr(hWnd, DWLP_USER);

			switch (LOWORD(wParam))
			{
				case IDC_OPT_FONT_CHANGE:
					ChangeFont(hWnd, poptctx);
					return(TRUE);

				case IDC_OK:
					SaveSettings(hWnd, poptctx);
				case IDC_CANCEL:
					end_dialog: EndDialog(hWnd, 0);
					return(TRUE);
			}

			break;
		}
	}

	return(FALSE);
}

VOID WINAPI OptionsDlgInit( HWND hWnd, POPTIONSCONTEXT poptctx )
{
	// Load strings
	{
		static const UINT16 arStrMap[][2] =
		{
			{ IDC_OPT_CM,             IDS_OPT_CM             },
			{ IDC_OPT_CM_ALWAYS,      IDS_OPT_CM_ALWAYS      },
			{ IDC_OPT_CM_EXTENDED,    IDS_OPT_CM_EXTENDED    },
			{ IDC_OPT_CM_NEVER,       IDS_OPT_CM_NEVER       },
			{ IDC_OPT_ENCODING,       IDS_OPT_ENCODING       },
			{ IDC_OPT_ENCODING_UTF8,  IDS_OPT_ENCODING_UTF8  },
			{ IDC_OPT_ENCODING_UTF16, IDS_OPT_ENCODING_UTF16 },
			{ IDC_OPT_ENCODING_ANSI,  IDS_OPT_ENCODING_ANSI  },
			{ IDC_OPT_FONT,           IDS_OPT_FONT           },
			{ IDC_OPT_FONT_CHANGE,    IDS_OPT_FONT_CHANGE    },
			{ IDC_OK,                 IDS_OPT_OK             },
			{ IDC_CANCEL,             IDS_OPT_CANCEL         }
		};

		static const TCHAR szFormat[] = TEXT("%s (v") TEXT(HASHCHECK_VERSION_STR) TEXT("/") TEXT(ARCH_NAME_PART) TEXT(")");

		TCHAR szBuffer[MAX_STRINGMSG], szTitle[MAX_STRINGMSG];
		UINT i;

		for (i = 0; i < countof(arStrMap); ++i)
			SetControlText(hWnd, arStrMap[i][0], arStrMap[i][1]);

		LoadString(g_hModThisDll, IDS_OPT_TITLE, szBuffer, countof(szBuffer));
		wnsprintf(szTitle, countof(szTitle), szFormat, szBuffer);
		SendMessage(hWnd, WM_SETTEXT, 0, (LPARAM)szTitle);
	}

	// Load options
	{
		poptctx->popt->dwFlags = HCOF_ALL;
		OptionsLoad(poptctx->popt);
		poptctx->popt->dwFlags = 0;

		SendDlgItemMessage(hWnd, IDC_OPT_CM_FIRSTID + poptctx->popt->dwMenuDisplay,
		                   BM_SETCHECK, BST_CHECKED, 0);

		SendDlgItemMessage(hWnd, IDC_OPT_ENCODING_FIRSTID + poptctx->popt->dwSaveEncoding,
		                   BM_SETCHECK, BST_CHECKED, 0);

		if (poptctx->hFont = CreateFontIndirect(&poptctx->popt->lfFont))
			SendDlgItemMessage(hWnd, IDC_OPT_FONT_PREVIEW, WM_SETFONT, (WPARAM)poptctx->hFont, FALSE);

		SetDlgItemText(hWnd, IDC_OPT_FONT_PREVIEW, poptctx->popt->lfFont.lfFaceName);
	}
}



/*============================================================================*\
	Dialog commands
\*============================================================================*/

VOID WINAPI ChangeFont( HWND hWnd, POPTIONSCONTEXT poptctx )
{
	HFONT hFont;

	CHOOSEFONT cf;
	cf.lStructSize = sizeof(cf);
	cf.hwndOwner = hWnd;
	cf.lpLogFont = &poptctx->popt->lfFont;
	cf.Flags = CF_FIXEDPITCHONLY | CF_FORCEFONTEXIST | CF_INITTOLOGFONTSTRUCT | CF_SCREENFONTS;

	if (ChooseFont(&cf) && (hFont = CreateFontIndirect(&poptctx->popt->lfFont)))
	{
		// Signal that a new font should be saved if "OK" is pressed
		poptctx->bFontChanged = TRUE;

		// Update the preview
		SendDlgItemMessage(hWnd, IDC_OPT_FONT_PREVIEW, WM_SETFONT, (WPARAM)hFont, FALSE);
		SetDlgItemText(hWnd, IDC_OPT_FONT_PREVIEW, poptctx->popt->lfFont.lfFaceName);

		// Clean up
		if (poptctx->hFont) DeleteObject(poptctx->hFont);
		poptctx->hFont = hFont;
	}
}

VOID WINAPI SaveSettings( HWND hWnd, POPTIONSCONTEXT poptctx )
{
	DWORD i;

	// Get the user-selected value for dwMenuDisplay
	for (i = 0; i < 3; ++i)
	{
		if (SendDlgItemMessage(hWnd, IDC_OPT_CM_FIRSTID + i, BM_GETCHECK, 0, 0) == BST_CHECKED)
			break;
	}

	// If the value has changed, flag it for saving
	if (i < 3 && poptctx->popt->dwMenuDisplay != i)
	{
		poptctx->popt->dwFlags |= HCOF_MENUDISPLAY;
		poptctx->popt->dwMenuDisplay = i;
	}

	// Get the user-selected value for dwSaveEncoding
	for (i = 0; i < 3; ++i)
	{
		if (SendDlgItemMessage(hWnd, IDC_OPT_ENCODING_FIRSTID + i, BM_GETCHECK, 0, 0) == BST_CHECKED)
			break;
	}

	// If the value has changed, flag it for saving
	if (i < 3 && poptctx->popt->dwSaveEncoding != i)
	{
		poptctx->popt->dwFlags |= HCOF_SAVEENCODING;
		poptctx->popt->dwSaveEncoding = i;
	}

	// Flag the font for saving if necessary
	if (poptctx->bFontChanged)
		poptctx->popt->dwFlags |= HCOF_FONT;

	OptionsSave(poptctx->popt);
}
