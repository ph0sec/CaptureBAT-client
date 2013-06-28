#include "ProcessManager.h"

ProcessManager::ProcessManager(void)
{
	cacheHits = 0;
	cacheMisses = 0;
	cacheFailures = 0;
	initialiseProcessMap();
}

ProcessManager::~ProcessManager(void)
{
	DebugPrint(L"Process Manager Statistics\n");
	DebugPrint(L"\tCache Hits: %i\n", cacheHits);
	DebugPrint(L"\tCache Misses: %i\n", cacheMisses);
	DebugPrint(L"\tCache Failures: %i\n", cacheFailures);
	instanceCreated = false;
	processMap.clear();
}

ProcessManager*
ProcessManager::getInstance()
{
	if(!instanceCreated)
	{
		processManager = new ProcessManager();
		instanceCreated = true;	
	}
	return processManager;
}

void
ProcessManager::onProcessEvent(BOOLEAN created, wstring time, 
							   DWORD parentProcessId,
							   DWORD processId, wstring process)
{
	// When a process is terminated we do not delete it from the processMap.
	// This is because there may be events that occur from the process after
	// it has terminated (because of the way the kernel driver monitors work)
	// We only delete when we encounter the same process id being created again
	if(created == TRUE)
	{
		//processMap.insert(ProcessPair(processId, process));
		insertProcess(processId, process);
	}
}

void 
ProcessManager::insertProcess(DWORD processId, wstring processPath)
{
	stdext::hash_map<DWORD, wstring>::iterator it;
	it = processMap.find(processId);
	if(it !=  processMap.end()) {
		processMap.erase(it);
	}
	processPath = convertFileObjectPathToDOSPath(processPath);
	DebugPrint(L"Capture-ProcessManager: Insert process %i -> %ls\n", processId, processPath.c_str()); 
	processMap.insert(ProcessPair(processId, processPath));
}

wstring
ProcessManager::convertDevicePathToDOSDrive(wstring devicePath)
{
	wchar_t drive = L'A';
	while(drive <= L'Z')
	{
		wchar_t szDriveName[3] = {drive,L':',L'\0'};
		wchar_t szTarget[512] = {0};
		if(QueryDosDevice(szDriveName, szTarget, 511) != 0)
			if(wcscmp(devicePath.c_str(), szTarget) == 0)
				return szDriveName;
		drive++;
	}
	return L"?:";
}

wstring
ProcessManager::convertFileObjectPathToDOSPath(wstring fileObjectPath)
{
	if(fileObjectPath.length() > 23)
	{
		wchar_t szTemp[2048];
		size_t pathOffset = fileObjectPath.find(L'\\', fileObjectPath.find(L'\\',1)+1);
		wstring devicePath = fileObjectPath.substr(0, pathOffset);
		wstring dosDrive = convertDevicePathToDOSDrive(devicePath);
		wstring dosPath = dosDrive;
		dosPath += fileObjectPath.substr(pathOffset);
		if(GetLongPathName(dosPath.c_str(),szTemp,2048) > 0)
		{
			dosPath = szTemp;
		}
		return dosPath;
	}
	return fileObjectPath;
}

wstring
ProcessManager::getProcessPath(DWORD processId)
{
	wstring processPath = L"UNKNOWN";
	if(processId == 4)
	{
		processPath = L"System";
		return processPath;
	}
	stdext::hash_map<DWORD, wstring>::iterator it;
	it = processMap.find(processId);
	if((it !=  processMap.end()) && (it->second != L"<unknown>"))
	{
		cacheHits++;
		processPath = it->second;
	} else {
		// NOTE testing showed that this case should never or hardly ever
		// occur. If you are getting a lot of cache misses maybe there is
		// something wrong during initialiseProcessMap() for example you
		// do not have the debug privilege.
		cacheMisses++;
		wchar_t szTemp[1024];
		ZeroMemory(&szTemp, sizeof(szTemp));

		HANDLE hProc = OpenProcess(PROCESS_QUERY_INFORMATION |
                            PROCESS_VM_READ, FALSE, processId);
		if(!hProc)
		{
			cacheFailures++;
			DebugPrint(L"Capture-ProcessManager: Cache failure - process id = %i\n", processId); 
			_ltow_s(processId, szTemp, 256, 10);
			processPath = szTemp;
			return processPath;
		}
		// Cannot use GetModuleFileName because the module list won't be
		// updated in time ... only happens on rare occasions.
		DWORD ret = GetProcessImageFileName(hProc, szTemp,1024);
		CloseHandle(hProc);
		if(ret > 0)
		{
			processPath = szTemp;
			DebugPrint(L"Capture-ProcessManager: Cache miss %i -> %ls\n", processId, szTemp); 
			insertProcess(processId, processPath);
		} else {
			cacheFailures++;
			DebugPrint(L"Capture-ProcessManager: Cache failure - process id = %i\n", processId); 
			_ltow_s(processId, szTemp, 256, 10);
			processPath = szTemp;
		}
	}
	
	return processPath;
}

wstring
ProcessManager::getProcessModuleName(DWORD processId)
{
	wstring processPath = getProcessPath(processId);
	wstring processModuleName = L"UNKNOWN";
	size_t moduleNameStart = processPath.find_last_of(L"\\");
	if(moduleNameStart != wstring::npos)
	{
		processModuleName = processPath.substr(moduleNameStart+1);
	}
	return processModuleName;
}

void
ProcessManager::initialiseProcessMap()
{
	DWORD aProcesses[1024], cbNeeded, cProcesses;

	if ( !EnumProcesses( aProcesses, sizeof(aProcesses), &cbNeeded ) )
		return;

	cProcesses = cbNeeded / sizeof(DWORD);
	for (UINT i = 0; i < cProcesses; i++ ) 
	{
		if( aProcesses[i] != 0 ) {
			DWORD processId;
			wchar_t processPathTemp[1024];
			wstring processPath = L"<unknown>";
			HANDLE hProcess = OpenProcess( PROCESS_QUERY_INFORMATION |
                               PROCESS_VM_READ,
                               FALSE, aProcesses[i] );
			processId = aProcesses[i];
			if (NULL != hProcess )
			{
				HMODULE hMod;
				DWORD cbNeeded;

				if ( EnumProcessModules( hProcess, &hMod, sizeof(hMod), 
						&cbNeeded) )
				{
					GetProcessImageFileName(hProcess, processPathTemp, 1024);
					processPath = processPathTemp;
				}
			}
			//printf("Added process: %i -> %ls\n", processId, processPath.c_str());
			//processMap.insert(ProcessPair(processId, processPath));
			insertProcess(processId, processPath);
			CloseHandle( hProcess );
		}
	}
}
