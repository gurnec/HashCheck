/**
 * HashCheck Shell Extension
 * Original work copyright (C) Kai Liu.  All rights reserved.
 * Modified work copyright (C) 2014, 2016 Christopher Gurnee.  All rights reserved.
 * Modified work copyright (C) 2016 Tim Schlueter.  All rights reserved.
 *
 * Please refer to readme.txt for information about this source code.
 * Please refer to license.txt for details about distribution and modification.
 **/

// Full name; this will appear in the version info and control panel
#define HASHCHECK_NAME_STR "HashCheck Shell Extension"

// Full version: MUST be in the form of major,minor,revision,build
#define HASHCHECK_VERSION_FULL 2,4,1,58

// String version: May be any suitable string
#define HASHCHECK_VERSION_STR "2.4.1.58-alpha"

#ifdef _USRDLL
// PE version: MUST be in the form of major.minor
#pragma comment(linker, "/version:2.4")
#endif

// Tail portion of the copyright string for the version resource
#define HASHCHECK_COPYRIGHT_STR "2008-2016 Kai Liu, Christopher Gurnee, Tim Schlueter, et al. All rights reserved."

// Name of the DLL
#define HASHCHECK_FILENAME_STR "HashCheck.dll"

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
