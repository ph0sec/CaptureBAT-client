/*
 *	PROJECT: Capture
 *	FILE: ProcessManager.h
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
#include "CaptureGlobal.h"
#include <string>
#include <hash_map>
#include <Psapi.h>

using namespace std;

typedef pair <DWORD, wstring> ProcessPair;

/*
	Class: ProcessManager
	
	Manages the process on the system. It provides a central place where objects can
	request the process paths of a process id. It is mainly used by the monitors so
	that they can accuratly retrieve the process path. When a process event occurs this
	the ProcessManager is informed and the path names of the created process are added
	to a map. When a object requests the process name it looks the id up in the map, if
	it is not found it will perform a GetImageFullPath() which is cached and if requested
	later will be returned. If it succeeds the cached path is returned. Testing has shown
	that GetImageFullPath is never called this is because the process monitor successfully
	receives a process event before the process actually does anything on the system.

	This is a singleton so only one object is available

	WARNIING/TODO Not threadsafe
*/
class ProcessManager
{
public:
	~ProcessManager(void);
	static ProcessManager* getInstance();

	wstring getProcessPath(DWORD processId);
	wstring getProcessModuleName(DWORD processId);
	void onProcessEvent(BOOLEAN created, wstring time, 
						DWORD parentProcessId,
						DWORD processId, wstring process);

	

	stdext::hash_map<DWORD, wstring> getProcessMap() { return processMap; }

private:
	static bool instanceCreated;
    static ProcessManager *processManager;
	
	ProcessManager(void);

	int cacheHits;
	int cacheMisses;
	int cacheFailures;

	void initialiseProcessMap();
	void insertProcess(DWORD processId, wstring processPath);
	wstring convertFileObjectPathToDOSPath(wstring fileObjectPath);
	wstring convertDevicePathToDOSDrive(wstring devicePath);

	stdext::hash_map<DWORD, wstring> processMap;
};
