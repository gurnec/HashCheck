/**
 * Given a font name, checks if that font is available on the system.
 * Copyright (C) Kai Liu.  All rights reserved.
 **/

#include <windows.h>

int CALLBACK EnumFontsProc( CONST LOGFONT *plf, CONST TEXTMETRIC *ptm,
                            DWORD FontType, LPARAM lParam )
{
	*((PBOOL)lParam) = TRUE;
	return(FALSE);
}

BOOL WINAPI IsFontAvailable( PCTSTR pszFaceName )
{
	// Yes, EnumFonts is old, but we neither need nor care about the additional
	// info returned by the newer font enumeration APIs; all that we care about
	// is whether the callback is ever called.

	BOOL fFound = FALSE;

	HDC hDC = GetDC(NULL);
	EnumFonts(hDC, pszFaceName, EnumFontsProc, (LPARAM)&fFound);
	ReleaseDC(NULL, hDC);

	return(fFound);
}
