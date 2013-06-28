#include "Analyzer.h"

Analyzer::Analyzer(Visitor* v, Server* s)
{
	processMonitor = new ProcessMonitor();
	registryMonitor = new RegistryMonitor();
	fileMonitor = new FileMonitor();
	collectModifiedFiles = false;
	captureNetworkPackets = false;
	networkPacketDumper = NULL;


	onOptionChangedConnection = OptionsManager::getInstance()->connect_onOptionChanged(boost::bind(&Analyzer::onOptionChanged, this, _1));


	visitor = v;
	visitor->onVisitEvent(boost::bind(&Analyzer::onVisitEvent, this, _1, _2, _3, _4));
	
	server = s;
	


	processMonitor->start();
	registryMonitor->start();
	fileMonitor->start();
}

Analyzer::~Analyzer(void)
{
	DebugPrint(L"Analyzer: Deconstructor");
	/* Do not change the order of these */
	/* The registry monitor must be deleted first ... as when the other monitors
	   kernel drivers are unloaded they cause a lot of registry events which, but
	   they are unloaded before these registry events are finished inside the
	   registry kernel driver. So ... when it comes to unloading the registry
	   kernel driver it will crash ... I guess. I'm not too sure on this as the
	   bug check has nothing to do with Capture instead it has something to do
	   with the PNP manager and deleting the unreferenced device */
	delete registryMonitor;
	delete processMonitor;
	delete fileMonitor;
	if(captureNetworkPackets)
	{
		delete networkPacketDumper;
	}
}

void
Analyzer::onOptionChanged(wstring option)
{
	wstring value = OptionsManager::getInstance()->getOption(option); 
	if(option == L"capture-network-packets") {
		if(value == L"true")
		{
			if(captureNetworkPackets == false)
			{
				printf("Creating network dumper\n");
				networkPacketDumper = new NetworkPacketDumper();
				captureNetworkPackets = true;
			}
		} else {
			if(captureNetworkPackets == true)
			{
				captureNetworkPackets = false;
				//if(networkPacketDumper != NULL)
				delete networkPacketDumper;			
			}
		}
	} else if(option == L"collect-modified-files") {
		if(value == L"true")
		{
			collectModifiedFiles = true;
		} else {
			collectModifiedFiles = false;
		}
		fileMonitor->setMonitorModifiedFiles(collectModifiedFiles);
	} else if(option == L"send-exclusion-lists") {
		if(value == L"true")
		{
			processMonitor->clearExclusionList();
			registryMonitor->clearExclusionList();
			fileMonitor->clearExclusionList();
		}
	}
}

void
Analyzer::start()
{
	malicious = false;
	DebugPrint(L"Analyzer: Start");
	if(captureNetworkPackets)
	{
		networkPacketDumper->start();
	}
	onProcessEventConnection = processMonitor->connect_onProcessEvent(boost::bind(&Analyzer::onProcessEvent, this, _1, _2, _3, _4, _5, _6));
	onRegistryEventConnection = registryMonitor->connect_onRegistryEvent(boost::bind(&Analyzer::onRegistryEvent, this, _1, _2, _3, _4));
	onFileEventConnection = fileMonitor->connect_onFileEvent(boost::bind(&Analyzer::onFileEvent, this, _1, _2, _3, _4));
	DebugPrint(L"Analyzer: Registered with callbacks");
	if(collectModifiedFiles)
	{
		fileMonitor->setMonitorModifiedFiles(true);
	}
}

void
Analyzer::stop()
{
	DebugPrint(L"Analyzer: Stop");
	onProcessEventConnection.disconnect();
	onRegistryEventConnection.disconnect();
	onFileEventConnection.disconnect();

	if(captureNetworkPackets)
	{
		networkPacketDumper->stop();
	}

	if(collectModifiedFiles || captureNetworkPackets)
	{
		SYSTEMTIME st;
		GetLocalTime(&st);
		if(collectModifiedFiles)
		{
			fileMonitor->setMonitorModifiedFiles(false);
			fileMonitor->copyCreatedFiles();
		}

		wchar_t* szLogFileName = new wchar_t[1024];

		wstring log = L"capture_";
		log += boost::lexical_cast<wstring>(st.wDay);
		log += boost::lexical_cast<wstring>(st.wMonth);
		log += boost::lexical_cast<wstring>(st.wYear);
		log += L"_";
		log += boost::lexical_cast<wstring>(st.wHour);
		log += boost::lexical_cast<wstring>(st.wMinute);
		log += L".zip";
		GetFullPathName(log.c_str(), 1024, szLogFileName, NULL);

		bool compressed = compressLogDirectory(szLogFileName);

		if(malicious)
		{
			if(server->isConnected() && compressed)
			{				
				FileUploader* uploader = new FileUploader(server);
				uploader->sendFile(szLogFileName);
				delete uploader;
				DeleteFile(szLogFileName);
			}
		}

		delete [] szLogFileName;
	}
}

wstring
Analyzer::errorCodeToString(DWORD errorCode)
{
	wchar_t szTemp[16];
	swprintf_s(szTemp, 16, L"%08x", errorCode);
	wstring error = szTemp;
	return error;
}


bool
Analyzer::compressLogDirectory(wstring logFileName)
{
	BOOL created = FALSE;
	STARTUPINFO siStartupInfo;
	wchar_t szTempPath[1024];
	PROCESS_INFORMATION piProcessInfo;

	DeleteFile(logFileName.c_str());

	memset(&siStartupInfo, 0, sizeof(siStartupInfo));
	memset(&piProcessInfo, 0, sizeof(piProcessInfo));
	ZeroMemory(&szTempPath, sizeof(szTempPath));

	siStartupInfo.cb = sizeof(siStartupInfo);

	_tcscat_s(szTempPath, 1024, L"\"");
	_tcscat_s(szTempPath, 1024, L"7za.exe");
	_tcscat_s(szTempPath, 1024, L"\" a -tzip -y \"");
	_tcscat_s(szTempPath, 1024, logFileName.c_str());
	_tcscat_s(szTempPath, 1024, L"\" logs");
	LPTSTR szCmdline=_tcsdup(szTempPath);
	created = CreateProcess(NULL,szCmdline, 0, 0, FALSE,
		CREATE_DEFAULT_ERROR_MODE, 0, 0, &siStartupInfo,
		&piProcessInfo);

	DWORD err = WaitForSingleObject( piProcessInfo.hProcess, INFINITE );

	//DebugPrint(L"Analyzer: compressLogDirectory - WaitForSingleObject = 0x%08x, CreateProcess=%i", err, created);
	if(!created)
	{
		printf("Analyzer: Cannot open 7za.exe process - 0x%08x\n", GetLastError());
		return false;
	} else {
		/* Delete the log directory */
		wchar_t* szFullPath = new wchar_t[1024];
		GetFullPathName(L"logs", 1024, szFullPath, NULL);
		SHFILEOPSTRUCT deleteLogDirectory;
		deleteLogDirectory.wFunc = FO_DELETE;
		deleteLogDirectory.pFrom = szFullPath;
		deleteLogDirectory.fFlags = FOF_NOCONFIRMATION | FOF_SILENT | FOF_NOERRORUI;
		SHFileOperation(&deleteLogDirectory);
		delete [] szFullPath;
		return true;
	}
}

void
Analyzer::sendVisitEvent(wstring* type, wstring* time, 
						 wstring* url, wstring* classification, wstring* application,
						 wstring* majorErrorCode, wstring* minorErrorCode)
{
	Attribute att;
	queue<Attribute> vAttributes;
	att.name = L"type";
	att.value = *type;
	vAttributes.push(att);
	att.name = L"time";
	att.value = *time;
	vAttributes.push(att);
	att.name = L"url";
	att.value = CaptureGlobal::urlEncode(*url);
	vAttributes.push(att);
	att.name = L"application";
	att.value = *application;
	vAttributes.push(att);
	att.name = L"malicious";
	att.value = *classification;
	vAttributes.push(att);
	att.name = L"major-error-code";
	att.value = *majorErrorCode;
	vAttributes.push(att);
	att.name = L"minor-error-code";
	att.value = *minorErrorCode;
	vAttributes.push(att);

	server->sendXML(L"visit-event", &vAttributes);
}

void
Analyzer::onVisitEvent(DWORD majorErrorCode, DWORD minorErrorCode, std::wstring url, std::wstring applicationPath)
{
	bool send = false;
	wstring type = L"";
	wstring classification = L"";
	wstring majErrorCode = L"";
	wstring minErrorCode = L"";

	if(majorErrorCode == CAPTURE_VISITATION_START) {
		type = L"start";
		send = true;
	} else if(majorErrorCode == CAPTURE_VISITATION_FINISH) {
		this->stop();
	} else if(majorErrorCode == CAPTURE_VISITATION_NETWORK_ERROR || 
		majorErrorCode == CAPTURE_VISITATION_TIMEOUT_ERROR)
	{
		type = L"error";
		classification = boost::lexical_cast<wstring>(malicious);
		majErrorCode = boost::lexical_cast<wstring>(majorErrorCode);
		minErrorCode = boost::lexical_cast<wstring>(minorErrorCode);
		send = true;
	} else if(majorErrorCode == CAPTURE_VISITATION_PRESTART) {
		this->start();
	} else if(majorErrorCode == CAPTURE_VISITATION_POSTFINISH) {	
		type = L"finish";
		classification = boost::lexical_cast<wstring>(malicious);
		send = true;
	} else if(majorErrorCode == CAPTURE_VISITATION_PROCESS_ERROR) {
		malicious = true;
		type = L"error";
		classification = boost::lexical_cast<wstring>(malicious);
		majErrorCode = boost::lexical_cast<wstring>(majorErrorCode);
		minErrorCode = boost::lexical_cast<wstring>(minorErrorCode);
		send = true;
	}

	if(send)
	{
		SYSTEMTIME st;

		GetLocalTime(&st);             // gets current time
		wstring time = L"";
		time += boost::lexical_cast<wstring>(st.wDay);
		time += L"/";
		time += boost::lexical_cast<wstring>(st.wMonth);
		time += L"/";
		time += boost::lexical_cast<wstring>(st.wYear);
		time += L" ";
		time += boost::lexical_cast<wstring>(st.wHour);
		time += L":";
		time += boost::lexical_cast<wstring>(st.wMinute);
		time += L".";
		time += boost::lexical_cast<wstring>(st.wMilliseconds);
		printf("%ls: %ls %ls %ls %ls\n", type.c_str(), url.c_str(), applicationPath.c_str(), majErrorCode.c_str(), minErrorCode.c_str());
		sendVisitEvent(&type, &time, &url, &classification, &applicationPath, &majErrorCode, &minErrorCode);
	}
}

void
Analyzer::sendSystemEvent(wstring* type, wstring* time, 
					wstring* process, wstring* action, 
					wstring* object)
{
	Attribute att;
	queue<Attribute> vAttributes;
	att.name = L"time";
	att.value = *time;
	vAttributes.push(att);
	att.name = L"type";
	att.value = *type;
	vAttributes.push(att);
	att.name = L"process";
	att.value = *process;
	vAttributes.push(att);
	att.name = L"action";
	att.value = *action;
	vAttributes.push(att);
	att.name = L"object";
	att.value = *object;
	vAttributes.push(att);
	if(OptionsManager::getInstance()->getOption(L"log-system-events-file") == L"")
	{
		// Output the event to stdout
		printf("%ls: %ls %ls -> %ls\n", type->c_str(), action->c_str(), process->c_str(), object->c_str());
	} else {
		// Send the event to the logger
		Logger::getInstance()->writeSystemEventToLog(type, time, process, action, object);
	}
	server->sendXML(L"system-event", &vAttributes);
}

void
Analyzer::onProcessEvent(BOOLEAN created, wstring time, 
						 DWORD parentProcessId, wstring parentProcess, 
						 DWORD processId, wstring process)
{
	malicious = true;
	wstring processEvent = L"process";
	wstring processType = L"";
	if(created == TRUE)
	{
		processType = L"created";
	} else {
		processType = L"terminated";
	}
	sendSystemEvent(&processEvent, &time, 
					&parentProcess, &processType, 
					&process);
}

void 
Analyzer::onRegistryEvent(wstring registryEventType, wstring time, 
						  wstring processPath, wstring registryEventPath)
{
	malicious = true;
	wstring registryEvent = L"registry";
	sendSystemEvent(&registryEvent, &time, 
					&processPath, &registryEventType, 
					&registryEventPath);
}

void
Analyzer::onFileEvent(wstring fileEventType, wstring time, 
						 wstring processPath, wstring fileEventPath)
{
	malicious = true;
	wstring fileEvent = L"file";
	sendSystemEvent(&fileEvent, &time, 
					&processPath, &fileEventType, 
					&fileEventPath);
}
