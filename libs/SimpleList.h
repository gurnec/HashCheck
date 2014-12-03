/**
 * SimpleList Library
 * Last modified: 2009/01/04
 * Copyright (C) Kai Liu.  All rights reserved.
 *
 * This library implements a highly efficient, fast, light-weight, and simple
 * list structure in C, with the goal of replacing the slower, heavier and
 * overly complex STL list.
 *
 * This list is intended as a write-once list; once an item has been added,
 * removing it or altering the data in a way that changes the size of the data
 * is unsupported.
 **/

#ifndef __SIMPLELIST_H__
#define __SIMPLELIST_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <windows.h>
#include "SimpleString.h"

/**
 * The SimpleList handle.
 **/

typedef PVOID HSIMPLELIST, *PHSIMPLELIST;

/**
 * SimpleList uses a mixture of __fastcall and __stdcall on x86-32, depending
 * on which is most appropriate for the situation.
 **/

#define SLSAPI __stdcall
#define SLFAPI __fastcall

/**
 * SLCreate: Returns the handle to a newly created and initialized list using
 * small heap-allocated blocks; NULL is returned if allocation failed.
 *
 * SLCreateEx: Returns the handle to a newly created and initialized list using
 * either small heap-allocated blocks or large high-granularity blocks; NULL
 * is returned if allocation failed.
 **/

#ifndef SL_ENABLE_LGBLK
HSIMPLELIST SLSAPI SLCreate( );
#else
#define SLCreate() SLCreateEx(FALSE)
HSIMPLELIST SLSAPI SLCreateEx( BOOL bLargeBlock );
#endif

/**
 * SLAddRef: Increments the reference counter; the initial value of the counter
 * after SLInit is 1, so there is no need to call SLAddRef after SLInit.
 *
 * SLRelease: Decrements the reference counter; SLRelease will destroy the list
 * and free the memory if the reference counter reaches zero, in which case,
 * the list handle will no longer be valid.
 **/

VOID SLFAPI SLAddRef( HSIMPLELIST hSimpleList );
VOID SLFAPI SLRelease( HSIMPLELIST hSimpleList );

/**
 * SLDestroy: Destroys the list and frees the memory, regardless of the current
 * reference count; this can be used in lieu of SLRelease if you do not care
 * about the reference count; this will invalidate the list handle.
 **/

VOID SLFAPI SLDestroy( HSIMPLELIST hSimpleList );

/**
 * SLReset: Resets the current position to the position of the first item.
 **/

VOID SLFAPI SLReset( HSIMPLELIST hSimpleList );

/**
 * SLCheck: Returns TRUE if the current item is valid; FALSE otherwise.
 **/

BOOL SLFAPI SLCheck( HSIMPLELIST hSimpleList );

/**
 * SLStep: Increments the current position; if the position cannot be advanced
 * (e.g., if the current position at the start of the call is past the end of
 * the list), then FALSE is returned.
 **/

BOOL SLFAPI SLStep( HSIMPLELIST hSimpleList );

/**
 * SLGetData*: Returns a pointer to the data of the current item; optionally,
 * the you can obtain the size of the data block, and the current position can
 * also be incremented after the data has been retrieved; NULL is returned if
 * the current position is invalid (e.g., past the end).
 *
 * NOTE: Do not free the pointer that was returned.
 **/

PVOID SLFAPI SLGetData( HSIMPLELIST hSimpleList );
PVOID SLFAPI SLGetDataEx( HSIMPLELIST hSimpleList, PUINT pcbData );
PVOID SLFAPI SLGetDataAndStep( HSIMPLELIST hSimpleList );
PVOID SLFAPI SLGetDataAndStepEx( HSIMPLELIST hSimpleList, PUINT pcbData );

/**
 * SLAddItem: Adds a block of data to the end of the list; a pointer to a
 * newly-created data block of cbData bytes long is returned if memory
 * allocation was successful (otherwise, NULL is returned).  NULL may be passed
 * in pvData if one wishes to use the returned pointer to copy the data.
 *
 * SLAddString(I): Adds a TCHAR string to the end of the list; a pointer to a
 * newly-created data block of cbData bytes long is returned if memory
 * allocation was successful (otherwise, NULL is returned).
 *
 * NOTE: Do not free the pointer that was returned.
 **/

PVOID SLFAPI SLAddItem( HSIMPLELIST hSimpleList, LPCVOID pvData, UINT cbData );
PVOID SLFAPI SLAddString( HSIMPLELIST hSimpleList, PCTSTR psz );

__forceinline PVOID SLAddStringI( HSIMPLELIST hSimpleList, PCTSTR psz )
{
	return(SLAddItem(hSimpleList, psz, ((UINT)SSLen(psz) + 1) * sizeof(TCHAR)));
}

/**
 * SLBuildIndex: Fills an array with the data pointers of every item; the array
 * is provided by the caller and must be at least cItems * sizeof(PVOID) bytes
 * in size.  The caller is thus also responsible for maintaining a count of the
 * number of elements in the list.  The current list position is NOT affected.
 **/

VOID SLFAPI SLBuildIndex( HSIMPLELIST hSimpleList, PVOID *ppvIndex );

/**
 * SLSetContextSize: Sets the amount of data allocated for an arbitrary block
 * of user data associated with the list; if successful, a pointer to the data
 * block is returned, otherwise, NULL is returned.  No memory is allocated for
 * the user data block when the list is first created.
 *
 * SLGetContextData: Returns a pointer to the user data block associated with
 * the list; NULL is returned if the data block had not been allocated.
 *
 * SLSetContextData: Copies data to the user data block associated with the
 * list and then returns a pointer to the data block; NULL is returned if the
 * data block had not been allocated.
 *
 * NOTE: Do not free the pointer that was returned; the pointer may also become
 * invalid after a SLRelease, SLDestroy, or SLSetContextSize call.
 **/

PVOID SLSAPI SLSetContextSize( HSIMPLELIST hSimpleList, UINT cbContextSize );
PVOID SLFAPI SLGetContextData( HSIMPLELIST hSimpleList );
PVOID SLFAPI SLSetContextData( HSIMPLELIST hSimpleList, LPCVOID pvData, UINT cbData );

#ifdef __cplusplus
}
#endif

#endif
