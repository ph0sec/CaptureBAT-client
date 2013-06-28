#include <Exdisp.h>
#include <comutil.h>
#include <exdispid.h>
#include <objbase.h>
#include <ole2.h>

/*	Thank you Micrsoft ... http://support.microsoft.com/kb/216686 */
HRESULT AutoWrap(int autoType, VARIANT *pvResult, IDispatch *pDisp, LPOLESTR ptName, int cArgs...) {
    // Begin variable-argument list...
    va_list marker;
    va_start(marker, cArgs);

    if(!pDisp) {
        MessageBox(NULL, L"NULL IDispatch passed to AutoWrap()", L"Error", 0x10010);
        _exit(0);
    }

    // Variables used...
    DISPPARAMS dp = { NULL, NULL, 0, 0 };
    DISPID dispidNamed = DISPID_PROPERTYPUT;
    DISPID dispID;
    HRESULT hr;
    //char buf[200];
    //char szName[200];

    
    // Convert down to ANSI
    //WideCharToMultiByte(CP_ACP, 0, ptName, -1, szName, 256, NULL, NULL);
    
    // Get DISPID for name passed...
    hr = pDisp->GetIDsOfNames(IID_NULL, &ptName, 1, LOCALE_USER_DEFAULT, &dispID);
    if(FAILED(hr)) {
       // sprintf(buf, "IDispatch::GetIDsOfNames(\"%s\") failed w/err 0x%08lx", szName, hr);
        MessageBox(NULL, L"1", L"AutoWrap1()", 0x10010);
        //_exit(0);
        return hr;
    }
    
    // Allocate memory for arguments...
    VARIANT *pArgs = new VARIANT[cArgs+1];
    // Extract arguments...
    for(int i=0; i<cArgs; i++) {
        pArgs[i] = va_arg(marker, VARIANT);
    }
    
    // Build DISPPARAMS
    dp.cArgs = cArgs;
    dp.rgvarg = pArgs;
    
    // Handle special-case for property-puts!
    if(autoType & DISPATCH_PROPERTYPUT) {
        dp.cNamedArgs = 1;
        dp.rgdispidNamedArgs = &dispidNamed;
    }
    
    // Make the call!
    hr = pDisp->Invoke(dispID, IID_NULL, LOCALE_SYSTEM_DEFAULT, autoType, &dp, pvResult, NULL, NULL);
    if(FAILED(hr)) {
		char buf[200];
        sprintf(buf, "IDispatch::Invoke(\"%s\"=%08lx) failed w/err 0x%08lx", L"blah", dispID, hr);
	  // printf(
        MessageBoxA(NULL, buf, "AutoWrap2()", 0x10010);
       // _exit(0);
        return hr;
    }
    // End variable-argument section...
    va_end(marker);
    
    delete [] pArgs;
    
    return hr;
}
	
	bool GetCOMServerExecutablePath(wstring COMApplicationName, wstring * path)
	{
		CLSID clsid;
		LPOLESTR pwszClsid;
		HKEY hKey;
		wchar_t szRegPath[1024];
		DWORD cSize = 1024;
		HRESULT hr = CLSIDFromProgID(COMApplicationName.c_str(), &clsid);
		if (FAILED(hr))
		{
			return false;
		}
		
		// Convert CLSID to String
		hr = StringFromCLSID(clsid, &pwszClsid);
		if (FAILED(hr))
		{
			return false;
		}

		wstring regPath = L"CLSID\\";
		regPath += pwszClsid;
		regPath += L"\\LocalServer32";

		LONG lRet = RegOpenKeyEx(HKEY_CLASSES_ROOT, regPath.c_str(), 0, KEY_ALL_ACCESS, &hKey);
		if (lRet != ERROR_SUCCESS) 
		{
			wstring regPath = L"CLSID\\";
			regPath += pwszClsid;
			regPath += L"\\LocalServer";
			lRet = RegOpenKeyEx(HKEY_CLASSES_ROOT, regPath.c_str(), 0, KEY_ALL_ACCESS, &hKey);
			if (lRet != ERROR_SUCCESS) 
			{
				return false;
			}
		}

		// Query value of key to get Path and close the key
		lRet = RegQueryValueEx(hKey, NULL, NULL, NULL, (LPBYTE)szRegPath, &cSize);
		RegCloseKey(hKey);
		if (lRet != ERROR_SUCCESS)
		{
			return false;
		} else {
			wchar_t *x = wcsrchr(szRegPath, '/');
			if(0!= x) // If no /Automation switch on the path
			{
				int result = x - szRegPath; 
				szRegPath[result]  = '\0'; 
			} 
			if(szRegPath[0] == '"')
			{
				
				wchar_t *c = wcsrchr(szRegPath, '"');
				if(0!= c)
				{
					int result = c - szRegPath; 
					szRegPath[result]  = '\0'; 
				}
				*path = wcsstr(szRegPath, L"\"")+1;
			} else {
				*path = szRegPath;
			}
			return true;
		}
	}