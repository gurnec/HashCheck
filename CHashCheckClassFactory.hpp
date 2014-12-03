/**
 * HashCheck Shell Extension
 * Copyright (C) Kai Liu.  All rights reserved.
 *
 * Please refer to readme.txt for information about this source code.
 * Please refer to license.txt for details about distribution and modification.
 **/

#ifndef __CHASHCHECKCLASSFACTORY_HPP__
#define __CHASHCHECKCLASSFACTORY_HPP__

#include "globals.h"

class CHashCheckClassFactory : public IClassFactory
{
	protected:
		CREF m_cRef;

	public:
		CHashCheckClassFactory( ) { InterlockedIncrement(&g_cRefThisDll); m_cRef = 1; }
		~CHashCheckClassFactory( ) { InterlockedDecrement(&g_cRefThisDll); }

		// IUnknown members
		STDMETHODIMP QueryInterface( REFIID, LPVOID * );
		STDMETHODIMP_(ULONG) AddRef( ) { return(InterlockedIncrement(&m_cRef)); }
		STDMETHODIMP_(ULONG) Release( )
		{
			// We need a non-volatile variable, hence the cRef variable
			LONG cRef = InterlockedDecrement(&m_cRef);
			if (cRef == 0) delete this;
			return(cRef);
		}

		// IClassFactory members
		STDMETHODIMP CreateInstance( LPUNKNOWN, REFIID, LPVOID * );
		STDMETHODIMP LockServer( BOOL ) { return(E_NOTIMPL); }
};

typedef CHashCheckClassFactory *LPCHASHCHECKCLASSFACTORY;

#endif
