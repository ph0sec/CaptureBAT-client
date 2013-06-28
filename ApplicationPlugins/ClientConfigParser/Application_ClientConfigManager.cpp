/*
 *	PROJECT: Capture
 *	FILE: Application_ClientConfigManager.cpp
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
#include "Application_ClientConfigManager.h"

Application_ClientConfigManager::Application_ClientConfigManager()
{
	loadApplicationsList();
}

Application_ClientConfigManager::~Application_ClientConfigManager(void)
{
	for(int i = 0; supportedApplications[i] != NULL; i++)
	{
		delete supportedApplications[i];
	}
	delete [] supportedApplications;
}

DWORD 
Application_ClientConfigManager::visitUrl(Url* url, DWORD* minorError)
{
	DWORD status = CAPTURE_VISITATION_OK;
	stdext::hash_map<wstring, APPLICATION*>::iterator it;
	it = applicationsMap.find(url->getApplicationName());
	if(it != applicationsMap.end())
	{	
		// Download file to temp directory if required

		FileDownloader* downloader = new FileDownloader();
		wstring file = url->getUrl().substr(url->getUrl().find_last_of(L"\\"));
		DWORD downloadStatus = 0;
		if((downloadStatus = downloader->Download(url->getUrl(), &file)) > 0)
		{
			*minorError = downloadStatus;
			return CAPTURE_VISITATION_NETWORK_ERROR;
		}


		PAPPLICATION pApplication = it->second;
		STARTUPINFO siStartupInfo;
		PROCESS_INFORMATION piProcessInfo;

		memset(&siStartupInfo, 0, sizeof(siStartupInfo));
		memset(&piProcessInfo, 0, sizeof(piProcessInfo));
		siStartupInfo.cb = sizeof(siStartupInfo);

		wstring processCommand = L"\"";
		processCommand += pApplication->path;
		processCommand += L"\" ";
		processCommand += url->getUrl();

		BOOL created = CreateProcess(NULL,(LPWSTR)processCommand.c_str(), 0, 0, FALSE,
			CREATE_DEFAULT_ERROR_MODE, 0, 0, &siStartupInfo,
			&piProcessInfo);

		if(created == FALSE)
		{
			status = CAPTURE_VISITATION_PROCESS_ERROR;
			*minorError = GetLastError();
		} else {
			Sleep(url->getVisitTime()*1000);
					
			bool processClosed = closeProcess(piProcessInfo.dwProcessId, minorError);
			if(!processClosed)
			{
				status = CAPTURE_VISITATION_PROCESS_ERROR;
			}
			Sleep(1000);
		}
	} else {
		status = CAPTURE_VISITATION_PROCESS_ERROR;
		*minorError = CAPTURE_PE_PROCESS_PATH_NOT_FOUND;
	}
	return status;
}

void 
Application_ClientConfigManager::loadApplicationsList()
{
	wstring line;
	int inserted = 0;
	wchar_t szFullPath[4096];
	GetFullPathName(L"Applications.conf", 4096, szFullPath, NULL);
	wifstream configF (szFullPath);
	if (!configF.is_open())
	{
		printf("Application_ConfigManager: Could not open the application config file: Applications.conf\n");
		supportedApplications = new wchar_t*[1];
		supportedApplications[0] = NULL;
		return;
	}
	while (!configF.eof())
	{
		getline (configF,line);
		if((line.size() > 0) && (line.at(0) != '#'))
		{
			vector<std::wstring> splitLine;			
			for(sf_it it=make_split_iterator(line, token_finder(is_any_of(L"\t")));
				it!=sf_it(); ++it)
			{
				splitLine.push_back(copy_range<std::wstring>(*it));				
			}

			if(splitLine.size() >= 2)
			{
				bool downloadURL = false;
				if(splitLine.size() == 3)
				{
					if(splitLine[2] == L"yes")
						downloadURL = true;
				}
				if(applicationsMap.find(splitLine[0]) == applicationsMap.end())
				{
					APPLICATION* a = new APPLICATION();
					a->name = splitLine[0];
					a->path = splitLine[1];
					a->downloadURL = downloadURL;
					applicationsMap.insert(ApplicationPair(splitLine[0], a));
					inserted++;
				}
			}
		}

	}
	supportedApplications = new wchar_t*[inserted+1];
	int i = 0;
	stdext::hash_map<wstring, PAPPLICATION>::iterator vit;
	for(vit = applicationsMap.begin(); vit != applicationsMap.end(); vit++)
	{
		supportedApplications[i] = new wchar_t[vit->first.length()+1];
		wcscpy_s(supportedApplications[i],vit->first.length()+1, vit->first.c_str());
		i++;
	}
	supportedApplications[i] = NULL;
	configF.close();
}

bool 
Application_ClientConfigManager::closeProcess(DWORD processId, DWORD* error)
{
	HANDLE hProcess = OpenProcess(PROCESS_TERMINATE,TRUE, processId);
	if(hProcess == NULL)
	{
		*error = GetLastError();
		if(*error == ERROR_INVALID_PARAMETER)
		{
			*error = CAPTURE_PE_PROCESS_ALREADY_TERMINATED;
		}
	} else {
		EnumWindows(Application_ClientConfigManager::EnumWindowsProc, (LPARAM)processId);

		DWORD tempProcessId = GetProcessId(hProcess);
		if(tempProcessId == processId)
		{
			if(!TerminateProcess(hProcess, 0))
			{
				*error = GetLastError();
			} else {
				*error = CAPTURE_PE_PROCESS_TERMINATED_FORCEFULLY;
			}
		} else {
			return true;
		}
	}
	return false;
}

BOOL CALLBACK 
Application_ClientConfigManager::EnumWindowsProc(HWND hwnd,LPARAM lParam)
{
	DWORD processId = (DWORD)lParam;
	if (GetWindowLong(hwnd,GWL_STYLE) & WS_VISIBLE) {
		DWORD windowsProcessId;
		GetWindowThreadProcessId(hwnd, &windowsProcessId);
		if (windowsProcessId == processId) 
		{
			WCHAR classname[256];
			GetClassName(hwnd, classname, sizeof(classname));
			HWND mainWindow = FindWindow(classname, NULL);
			SendMessage(mainWindow, WM_CLOSE, 1, 0);
		}
	}
	return TRUE;
}
