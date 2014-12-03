/**
 * Given a font name, checks if that font is available on the system.
 * Copyright (C) Kai Liu.  All rights reserved.
 **/

#ifndef __ISFONTAVAILABLE_H__
#define __ISFONTAVAILABLE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <windows.h>

BOOL WINAPI IsFontAvailable( PCTSTR pszFaceName );

#ifdef __cplusplus
}
#endif

#endif
