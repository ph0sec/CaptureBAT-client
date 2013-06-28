/*
 *	PROJECT: Capture
 *	FILE: FileDownloader.h
 *	AUTHORS: Ramon Steenson (rsteenson@gmail.com) & Christian Seifert (christian.seifert@gmail.com)
 *
 *	Developed by Victoria University of Wellington and the New Zealand Honeynet Alliance
 *
 *	This file is part of Capture.
 *
 *	Capture is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Capture is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Capture; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */
#pragma once
#include <Urlmon.h>
#include <string>

#include "../../ErrorCodes.h"

using namespace std;

/*
	Class: FileDownloader
	
	Downloads files from the internet into a temporary directory. Used mainly by the
	<Visitor> component for applications which cannot open remote files (Adobe acrobat
	for example)

	Beware that this uses some MFC style programming ...
*/
class FileDownloader : public IBindStatusCallback
{
public:
	FileDownloader(void);
	~FileDownloader(void);

	DWORD Download(wstring what, wstring *to);

    HRESULT STDMETHODCALLTYPE OnProgress( 
            /* [in] */ ULONG ulProgress,
            /* [in] */ ULONG ulProgressMax,
            /* [in] */ ULONG ulStatusCode,
            /* [in] */ LPCWSTR szStatusText)
	{
		if(ulStatusCode == BINDSTATUS_DOWNLOADINGDATA)
		{
			if(!bDownloading)
			{
				bDownloading = true;
				SetEvent(hConnected);
			}
		} else if(ulStatusCode == BINDSTATUS_ENDDOWNLOADDATA)
		{
			if(!bDownloading)
			{
				bDownloading = true;
				SetEvent(hConnected);
			}
			SetEvent(hDownloaded);
		}
		return(NOERROR);
	}

	HRESULT STDMETHODCALLTYPE OnStartBinding( 
            /* [in] */ DWORD dwReserved,
            /* [in] */ IBinding *pib)
	{
		return(E_NOTIMPL);
	}
        
    HRESULT STDMETHODCALLTYPE GetPriority( 
            /* [out] */ LONG *pnPriority)
	{
		return(E_NOTIMPL);
	}
        
    HRESULT STDMETHODCALLTYPE OnLowResource( 
            /* [in] */ DWORD reserved)
	{
		return(E_NOTIMPL);
	}
        
    HRESULT STDMETHODCALLTYPE OnStopBinding( 
            /* [in] */ HRESULT hresult,
            /* [unique][in] */ LPCWSTR szError)
	{
		return(E_NOTIMPL);
	}
        
    HRESULT STDMETHODCALLTYPE GetBindInfo( 
            /* [out] */ DWORD *grfBINDF,
            /* [unique][out][in] */ BINDINFO *pbindinfo)
	{
		return(E_NOTIMPL);
	}
        
    HRESULT STDMETHODCALLTYPE OnDataAvailable( 
            /* [in] */ DWORD grfBSCF,
            /* [in] */ DWORD dwSize,
            /* [in] */ FORMATETC *pformatetc,
            /* [in] */ STGMEDIUM *pstgmed)
	{
		return(E_NOTIMPL);
	}
        
    HRESULT STDMETHODCALLTYPE OnObjectAvailable( 
            /* [in] */ REFIID riid,
            /* [iid_is][in] */ IUnknown *punk)
	{
		return(E_NOTIMPL);
	}

	HRESULT STDMETHODCALLTYPE QueryInterface( 
                /* [in] */ REFIID riid,
                /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject)
	{
		return(E_NOTIMPL);
	}
            
	ULONG STDMETHODCALLTYPE AddRef( void)
	{
		return(E_NOTIMPL);
	}
            
	ULONG STDMETHODCALLTYPE Release( void)
	{
		return(E_NOTIMPL);
	}
private:
	HANDLE hDownloaded;
	HANDLE hConnected;
	bool bDownloading;
};
