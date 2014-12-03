/**
 * SimpleList Library
 * Last modified: 2009/01/04
 * Copyright (C) Kai Liu.  All rights reserved.
 **/

#include "WinIntrinsics.h"
#include "SimpleList.h"

/**
 * Control structures
 **/

typedef struct {
	PVOID pvNext;           // address of the next item; NULL if there is none
	UINT cbData;            // size, in bytes, of the current item
	UINT_PTR data[];        // properly aligned start of data
} SLITEM, *PSLITEM;

typedef struct {
	PVOID pvPrev;           // address of the previous memory block; NULL if there is none
	UINT_PTR data[];        // properly aligned start of data
} SLBLOCK, *PSLBLOCK;

typedef struct {
	PVOID pvContext;        // optional user context data associated with this list
	PSLITEM pItemStart;     // the first item; NULL if list is empty
	PSLITEM pItemCurrent;   // the current item (used only by read/step)
	PSLITEM pItemLast;      // the last item; NULL if list is empty (used only by add)
	PSLITEM pItemNew;       // the tail (used only by add)
	PSLBLOCK pBlockCurrent; // the current block to which new items will be added
	UINT cbRemaining;       // bytes remaining in the current block
	#ifdef SL_ENABLE_LGBLK
	UINT cbBlockSize;       // total bytes in each block
	BOOL bLargeBlock;       // use large blocks allocated by VirtualAlloc?
	#endif
	CREF cRef;              // 0-based reference count (0 == 1 reference, 1 == 2 references, etc.)
} SLHEADER, *PSLHEADER;

/**
 * Basic heap memory management functions
 **/

#ifndef SL_FORCE_REALLOC
#define SLInternal_Malloc               malloc
#define SLInternal_Free                 free
#define SLInternal_Realloc              realloc
#else
#define SLInternal_Malloc(cb)           realloc(NULL, cb)
#define SLInternal_Free(pv)             realloc(pv, 0)
#define SLInternal_Realloc              realloc
#endif

/**
 * Memory block management functions
 **/

#ifndef SL_SMBLK_SIZE
#define SL_SMBLK_SIZE 0x800
#endif

#ifndef SL_ENABLE_LGBLK
#define SL_BLOCK_SIZE                   SL_SMBLK_SIZE
#define SLInternal_AllocBlock(hdr, cb)  SLInternal_Malloc(cb)
#define SLInternal_FreeBlock(hdr, pv)   SLInternal_Free(pv)
#else
#define SL_BLOCK_SIZE                   pHeader->cbBlockSize

__forceinline PVOID SLInternal_AllocBlock( PSLHEADER pHeader, UINT cbSize )
{
	if (pHeader->bLargeBlock)
		return(VirtualAlloc(NULL, cbSize, MEM_COMMIT, PAGE_READWRITE));
	else
		return(SLInternal_Malloc(cbSize));
}

__forceinline VOID SLInternal_FreeBlock( PSLHEADER pHeader, PVOID pvBlock )
{
	if (pHeader->bLargeBlock)
		VirtualFree(pvBlock, 0, MEM_RELEASE);
	else
		SLInternal_Free(pvBlock);
}

__forceinline UINT SLInternal_GetBlockSize( PSLHEADER pHeader )
{
	if (pHeader->bLargeBlock)
	{
		SYSTEM_INFO si;
		GetSystemInfo(&si);
		return(si.dwAllocationGranularity);
	}
	else
	{
		return(SL_SMBLK_SIZE);
	}
}

#endif

/**
 * Internal helper functions
 **/

__forceinline VOID SLInternal_Destroy( PSLHEADER pHeader )
{
	PSLBLOCK pBlock, pBlockPrev;

	for (pBlock = pHeader->pBlockCurrent; pBlock; pBlock = pBlockPrev)
	{
		pBlockPrev = pBlock->pvPrev;
		SLInternal_FreeBlock(pHeader, pBlock);
	}

	SLInternal_Free(pHeader->pvContext);
	SLInternal_Free(pHeader);
}

__forceinline BOOL SLInternal_Check( PSLHEADER pHeader )
{
	return(pHeader && pHeader->pItemCurrent);
}

__forceinline UINT SLInternal_Align( UINT uData )
{
	if (sizeof(UINT_PTR) == 4 || sizeof(UINT_PTR) == 8)
	{
		// Round up to the nearest pointer-sized multiple
		return((uData + (sizeof(UINT_PTR) - 1)) & ~(sizeof(UINT_PTR) - 1));
	}
	else
	{
		// In the highly unlikely event that this code is compiled on some
		// architecture that does not have a 4- or 8-byte word size, here is
		// a more general alignment formula; this entire branch will be culled
		// by the compiler's optimizer on normal architectures
		return(((uData + (sizeof(UINT_PTR) - 1)) / sizeof(UINT_PTR)) * sizeof(UINT_PTR));
	}
}

/**
 * SLCreate
 * SLCreateEx
 **/

#ifndef SL_ENABLE_LGBLK

HSIMPLELIST SLSAPI SLCreate( )
{
	PSLHEADER pHeader = SLInternal_Malloc(sizeof(SLHEADER));

	if (pHeader)
	{
		// Since cRef is a 0-based counter (vs. the usual 1-based counter), the
		// initial value of 0 from ZeroMemory is correct
		ZeroMemory(pHeader, sizeof(SLHEADER));
	}

	return(pHeader);
}

#else

HSIMPLELIST SLSAPI SLCreateEx( BOOL bLargeBlock )
{
	PSLHEADER pHeader = SLInternal_Malloc(sizeof(SLHEADER));

	if (pHeader)
	{
		// Since cRef is a 0-based counter (vs. the usual 1-based counter), the
		// initial value of 0 from ZeroMemory is correct
		ZeroMemory(pHeader, sizeof(SLHEADER));

		pHeader->bLargeBlock = bLargeBlock;
		pHeader->cbBlockSize = SLInternal_GetBlockSize(pHeader);
	}

	return(pHeader);
}

#endif

/**
 * SLAddRef
 * SLRelease
 **/

VOID SLFAPI SLAddRef( HSIMPLELIST hSimpleList )
{
	CONST PSLHEADER pHeader = (PSLHEADER)hSimpleList;

	if (pHeader)
		InterlockedIncrement(&pHeader->cRef);
}

VOID SLFAPI SLRelease( HSIMPLELIST hSimpleList )
{
	CONST PSLHEADER pHeader = (PSLHEADER)hSimpleList;

	if (pHeader)
	{
		if (InterlockedDecrement(&pHeader->cRef) < 0)
			SLInternal_Destroy(pHeader);
	}
}

/**
 * SLDestroy
 **/

VOID SLFAPI SLDestroy( HSIMPLELIST hSimpleList )
{
	CONST PSLHEADER pHeader = (PSLHEADER)hSimpleList;

	if (pHeader)
		SLInternal_Destroy(pHeader);
}

/**
 * SLReset
 **/

VOID SLFAPI SLReset( HSIMPLELIST hSimpleList )
{
	CONST PSLHEADER pHeader = (PSLHEADER)hSimpleList;

	if (pHeader)
		pHeader->pItemCurrent = pHeader->pItemStart;
}

/**
 * SLCheck
 **/

BOOL SLFAPI SLCheck( HSIMPLELIST hSimpleList )
{
	return(SLInternal_Check((PSLHEADER)hSimpleList));
}

/**
 * SLStep
 **/

BOOL SLFAPI SLStep( HSIMPLELIST hSimpleList )
{
	CONST PSLHEADER pHeader = (PSLHEADER)hSimpleList;

	if (SLInternal_Check(pHeader))
	{
		pHeader->pItemCurrent = pHeader->pItemCurrent->pvNext;
		return(TRUE);
	}

	return(FALSE);
}

/**
 * SLGetData
 * SLGetDataEx
 * SLGetDataAndStep
 * SLGetDataAndStepEx
 **/

PVOID SLFAPI SLGetData( HSIMPLELIST hSimpleList )
{
	CONST PSLHEADER pHeader = (PSLHEADER)hSimpleList;

	if (SLInternal_Check(pHeader))
	{
		PVOID pvData = pHeader->pItemCurrent->data;
		return(pvData);
	}

	return(NULL);
}

PVOID SLFAPI SLGetDataEx( HSIMPLELIST hSimpleList, PUINT pcbData )
{
	CONST PSLHEADER pHeader = (PSLHEADER)hSimpleList;

	if (SLInternal_Check(pHeader))
	{
		PVOID pvData = pHeader->pItemCurrent->data;
		*pcbData = pHeader->pItemCurrent->cbData;
		return(pvData);
	}

	return(NULL);
}

PVOID SLFAPI SLGetDataAndStep( HSIMPLELIST hSimpleList )
{
	CONST PSLHEADER pHeader = (PSLHEADER)hSimpleList;

	if (SLInternal_Check(pHeader))
	{
		PVOID pvData = pHeader->pItemCurrent->data;
		pHeader->pItemCurrent = pHeader->pItemCurrent->pvNext;
		return(pvData);
	}

	return(NULL);
}

PVOID SLFAPI SLGetDataAndStepEx( HSIMPLELIST hSimpleList, PUINT pcbData )
{
	CONST PSLHEADER pHeader = (PSLHEADER)hSimpleList;

	if (SLInternal_Check(pHeader))
	{
		PVOID pvData = pHeader->pItemCurrent->data;
		*pcbData = pHeader->pItemCurrent->cbData;
		pHeader->pItemCurrent = pHeader->pItemCurrent->pvNext;
		return(pvData);
	}

	return(NULL);
}

/**
 * SLAddItem
 * SLAddString
 **/

PVOID SLFAPI SLAddItem( HSIMPLELIST hSimpleList, LPCVOID pvData, UINT cbData )
{
	CONST PSLHEADER pHeader = (PSLHEADER)hSimpleList;

	// Note that adding items larger than the block size is not supported
	CONST UINT cbRequired = SLInternal_Align(sizeof(SLITEM) + cbData);

	if (pHeader && cbRequired <= SL_BLOCK_SIZE - sizeof(SLBLOCK))
	{
		if (cbRequired > pHeader->cbRemaining)
		{
			PSLBLOCK pBlockNew = SLInternal_AllocBlock(pHeader, SL_BLOCK_SIZE);

			if (pBlockNew)
			{
				pBlockNew->pvPrev = pHeader->pBlockCurrent;
				pHeader->pItemNew = (PSLITEM)pBlockNew->data;
				pHeader->pBlockCurrent = pBlockNew;
				pHeader->cbRemaining = SL_BLOCK_SIZE - sizeof(SLBLOCK);
			}
			else
			{
				// Allocation failure
				return(NULL);
			}
		}

		// Initialize the new SLITEM
		pHeader->pItemNew->pvNext = NULL;
		pHeader->pItemNew->cbData = cbData;
		if (pvData) memcpy(pHeader->pItemNew->data, pvData, cbData);

		if (pHeader->pItemStart == NULL)
		{
			// If this is the first item in the list...
			pHeader->pItemStart = pHeader->pItemNew;
			pHeader->pItemCurrent = pHeader->pItemNew;
		}
		else
		{
			// If this is not the first item, then link it to the previous item
			pHeader->pItemLast->pvNext = pHeader->pItemNew;
		}

		pHeader->pItemLast = pHeader->pItemNew;
		pHeader->pItemNew = (PSLITEM)((PBYTE)pHeader->pItemNew + cbRequired);
		pHeader->cbRemaining -= cbRequired;

		return(pHeader->pItemLast->data);
	}

	return(NULL);
}

PVOID SLFAPI SLAddString( HSIMPLELIST hSimpleList, PCTSTR psz )
{
	return(SLAddStringI(hSimpleList, psz));
}

/**
 * SLBuildIndex
 **/

VOID SLFAPI SLBuildIndex( HSIMPLELIST hSimpleList, PVOID *ppvIndex )
{
	CONST PSLHEADER pHeader = (PSLHEADER)hSimpleList;

	if (pHeader)
	{
		PSLITEM pItem;

		for (pItem = pHeader->pItemStart; pItem; pItem = pItem->pvNext)
		{
			*ppvIndex = pItem->data;
			++ppvIndex;
		}
	}
}

/**
 * SLSetContextSize
 * SLGetContextData
 * SLSetContextData
 **/

PVOID SLSAPI SLSetContextSize( HSIMPLELIST hSimpleList, UINT cbContextSize )
{
	CONST PSLHEADER pHeader = (PSLHEADER)hSimpleList;

	if (pHeader)
	{
		PVOID pvNew = SLInternal_Realloc(pHeader->pvContext, cbContextSize);

		if (pvNew || cbContextSize == 0)
			return(pHeader->pvContext = pvNew);
	}

	return(NULL);
}

PVOID SLFAPI SLGetContextData( HSIMPLELIST hSimpleList )
{
	CONST PSLHEADER pHeader = (PSLHEADER)hSimpleList;

	if (pHeader)
		return(pHeader->pvContext);

	return(NULL);
}

PVOID SLFAPI SLSetContextData( HSIMPLELIST hSimpleList, LPCVOID pvData, UINT cbData )
{
	CONST PSLHEADER pHeader = (PSLHEADER)hSimpleList;

	if (pHeader && pHeader->pvContext)
		return(memcpy(pHeader->pvContext, pvData, cbData));

	return(NULL);
}
