/**
 * HashCheck Shell Extension
 * Copyright (C) Kai Liu.  All rights reserved.
 *
 * Please refer to readme.txt for information about this source code.
 * Please refer to license.txt for details about distribution and modification.
 **/

#include "CHashCheckClassFactory.hpp"
#include "CHashCheck.hpp"

STDMETHODIMP CHashCheckClassFactory::QueryInterface( REFIID riid, LPVOID *ppv )
{
	if (IsEqualIID(riid, IID_IUnknown))
	{
		*ppv = this;
	}
	else if (IsEqualIID(riid, IID_IClassFactory))
	{
		*ppv = (LPCLASSFACTORY)this;
	}
	else
	{
		*ppv = NULL;
		return(E_NOINTERFACE);
	}

	AddRef();
	return(S_OK);
}

STDMETHODIMP CHashCheckClassFactory::CreateInstance( LPUNKNOWN pUnkOuter, REFIID riid, LPVOID *ppv )
{
	*ppv = NULL;

	if (pUnkOuter) return(CLASS_E_NOAGGREGATION);

	LPCHASHCHECK lpHashCheck = new CHashCheck;
	if (lpHashCheck == NULL) return(E_OUTOFMEMORY);

	HRESULT hr = lpHashCheck->QueryInterface(riid, ppv);
	lpHashCheck->Release();
	return(hr);
}
