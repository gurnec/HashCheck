/**
 * HashCheck Shell Extension
 * Copyright (C) Kai Liu.  All rights reserved.
 *
 * Please refer to readme.txt for information about this source code.
 * Please refer to license.txt for details about distribution and modification.
 **/

#ifndef __HASHCHECKOPTIONS_H__
#define __HASHCHECKOPTIONS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <windows.h>

// Options struct
typedef struct {
	DWORD dwFlags;
	DWORD dwFilterIndex;
	DWORD dwMenuDisplay;
	DWORD dwSaveEncoding;
	LOGFONT lfFont;
} HASHCHECKOPTIONS, *PHASHCHECKOPTIONS;

// Options flags
#define HCOF_FILTERINDEX  0x00000001  // The dwFilterIndex member is valid
#define HCOF_MENUDISPLAY  0x00000002  // The dwMenuDisplay member is valid
#define HCOF_SAVEENCODING 0x00000004  // The dwSaveEncoding member is valid
#define HCOF_FONT         0x00000008  // The lfFont member is valid
#define HCOF_ALL          0x0000000F

// Public functions
VOID __fastcall OptionsDialog( HWND hWndOwner, PHASHCHECKOPTIONS popt );
VOID __fastcall OptionsLoad( PHASHCHECKOPTIONS popt );
VOID __fastcall OptionsSave( PHASHCHECKOPTIONS popt );

#ifdef __cplusplus
}
#endif

#endif
