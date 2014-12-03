/**
 * HashCheck Shell Extension
 * Copyright (C) Kai Liu.  All rights reserved.
 *
 * Please refer to readme.txt for information about this source code.
 * Please refer to license.txt for details about distribution and modification.
 **/

// Full name; this will appear in the version info and control panel
#define HASHCHECK_NAME_STR "HashCheck Shell Extension"

// Full version: MUST be in the form of major,minor,revision,build
#define HASHCHECK_VERSION_FULL 2,1,11,1

// String version: May be any suitable string
#define HASHCHECK_VERSION_STR "2.1.11.1"

#ifdef _USRDLL
// PE version: MUST be in the form of major.minor
#pragma comment(linker, "/version:2.1")
#endif

// String used in the "CompanyName" field of the version resource
#define HASHCHECK_AUTHOR_STR "code.kliu.org"

// Tail portion of the copyright string for the version resource
#define HASHCHECK_COPYRIGHT_STR "Kai Liu. All rights reserved."

// Name of the DLL
#define HASHCHECK_FILENAME_STR "HashCheck.dll"

// String used for setup
#define HASHCHECK_SETUP_STR "HashCheck Shell Extension Setup"

// Architecture names
#if defined(_M_IX86)
#define ARCH_NAME_TAIL " (x86-32)"
#define ARCH_NAME_FULL "x86-32"
#define ARCH_NAME_PART "x86"
#elif defined(_M_AMD64) || defined(_M_X64)
#define ARCH_NAME_TAIL " (x86-64)"
#define ARCH_NAME_FULL "x86-64"
#define ARCH_NAME_PART "x64"
#elif defined(_M_IA64)
#define ARCH_NAME_TAIL " (IA-64)"
#define ARCH_NAME_FULL "IA-64"
#define ARCH_NAME_PART ARCH_NAME_FULL
#else
#define ARCH_NAME_TAIL ""
#define ARCH_NAME_FULL "Unspecified"
#define ARCH_NAME_PART ARCH_NAME_FULL
#endif
