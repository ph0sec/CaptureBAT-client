#include <iostream>
#include <boost/signal.hpp>
#include <boost/bind.hpp>
#include "Server.h"
#include "Visitor.h"
#include "Analyzer.h"
#include "ProcessManager.h"
#include "shellapi.h"
using namespace std;

/* Initialise static variables. These are all singletons */
/* Process manager which keeps track of opened processes and their paths */
bool ProcessManager::instanceCreated = false;
ProcessManager* ProcessManager::processManager = NULL;

/* Objects listen for internal events from this controller */
bool EventController::instanceCreated = false;
EventController* EventController::pEventController = NULL;

/* Logging capabilities that either writes to a file or console */
bool Logger::instanceCreated = false;
Logger* Logger::logger = NULL;

/* Global options available to all objects */
bool OptionsManager::instanceCreated = false;
OptionsManager* OptionsManager::optionsManager = NULL;

class CaptureClient : public Runnable
{
public:
	CaptureClient()
	{
		/* Capture needs the SeDriverLoadPrivilege and SeDebugPrivilege to correctly run.
		   If it doesn't acquire these privileges it exits. DebugPrivilege is used in the
		   ProcessManager to open handles to existing process. DriverLoadPrivilege allows
		   Capture to load the kernel drivers. */
		if(!obtainDebugPrivilege() || !obtainDriverLoadPrivilege())
		{	
			printf("\n\nCould not acquire privileges. Make sure you are running Capture with Administrator rights\n");
			exit(1);
		}

		/* Create the log directories */
		CreateDirectory(L"logs",NULL);

		/* If logging to a file has been specified open the file for write access */
		if(OptionsManager::getInstance()->getOption(L"log-system-events-file") != L"")
			Logger::getInstance()->openLogFile(OptionsManager::getInstance()->getOption(L"log-system-events-file"));

		hStopRunning = CreateEvent(NULL, FALSE, FALSE, NULL);
		wstring serverIp = OptionsManager::getInstance()->getOption(L"server");
		server = new Server(serverIp, 7070);
		server->onConnectionStatusChanged(boost::bind(&CaptureClient::onConnectionStatusChanged, this, _1));

		/* Listen for events from the EventController. These are messages passed from the server */
		onServerConnectEventConnection = EventController::getInstance()->connect_onServerEvent(L"connect", boost::bind(&CaptureClient::onServerConnectEvent, this, _1));
		onServerPingEventConnection = EventController::getInstance()->connect_onServerEvent(L"ping", boost::bind(&CaptureClient::onServerPingEvent, this, _1));

		/* Start running the Capture Client */
		visitor = new Visitor();
		analyzer =  new Analyzer(visitor, server);
		Thread* captureClientThread = new Thread(this);
		captureClientThread->start("CaptureClient");
	}
	
	~CaptureClient()
	{
		onServerConnectEventConnection.disconnect();
		onServerPingEventConnection.disconnect();
		CloseHandle(hStopRunning);
		delete analyzer;
		delete visitor;
		delete server;
		
		
		Logger::getInstance()->closeLogFile();
		delete Logger::getInstance();
		delete ProcessManager::getInstance();
		delete OptionsManager::getInstance();
		delete EventController::getInstance();
	}

	void run()
	{		
		if((OptionsManager::getInstance()->getOption(L"server") == L"") || 
			server->connectToServer())
		{
			/* If Capture is being run in standalone mode start the analyzer now */
			/* Send file test */
			printf("---------------------------------------------------------\n");
			if((OptionsManager::getInstance()->getOption(L"server") == L""))
			{
				analyzer->start();
			}
			/* Wait till a user presses a key then exit */
			getchar();
			if((OptionsManager::getInstance()->getOption(L"server") == L""))
			{
				analyzer->stop();
			}
		}
		SetEvent(hStopRunning);
	}

	/* Event passed when the state of the connection between the server is changed.
	   When capture becomes disconnected from the server the client exits */
	void onConnectionStatusChanged(bool connectionStatus)
	{
		printf("Got connect status changed\n");
		if(!connectionStatus)
		{
			if(OptionsManager::getInstance()->getOption(L"server") != L"")
				printf("Disconnected from server: Exiting client\n");
			SetEvent(hStopRunning);
		}
	}

	/* Responds to a connect event from the server. It sends the virtual machine
	   server id and virtual machine id this client is hosted on back to the server */
	void onServerConnectEvent(Element* pElement)
	{
		printf("Got connect event\n");
		if(pElement->name == L"connect")
		{
			Attribute att;
			queue<Attribute> vAttributes;
			att.name = L"vm-server-id";
			att.value = OptionsManager::getInstance()->getOption(L"vm-server-id");
			vAttributes.push(att);
			att.name = L"vm-id";
			att.value = OptionsManager::getInstance()->getOption(L"vm-id");
			vAttributes.push(att);
			server->sendXML(L"connect", &vAttributes);
		}
	}

	/* Sends a pong message back to the server when a ping is received */
	void onServerPingEvent(Element* pElement)
	{
		if(pElement->name == L"ping")
		{
			queue<Attribute> vAttributes;
			server->sendXML(L"pong", &vAttributes);
		}
	}

	/* Get the driver load privilege so that the kernel drivers can be loaded */
	bool obtainDriverLoadPrivilege()
	{
		HANDLE hToken;
		if(OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,&hToken))
		{
			TOKEN_PRIVILEGES tp;
			LUID luid;
			LPCTSTR lpszPrivilege = L"SeLoadDriverPrivilege";

			if ( !LookupPrivilegeValue( 
					NULL,            // lookup privilege on local system
					lpszPrivilege,   // privilege to lookup 
					&luid ) )        // receives LUID of privilege
			{
				printf("LookupPrivilegeValue error: %u\n", GetLastError() ); 
				return false; 
			}

			tp.PrivilegeCount = 1;
			tp.Privileges[0].Luid = luid;
			tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

			// Enable the privilege or disable all privileges.
			if ( !AdjustTokenPrivileges(
				hToken, 
				FALSE, 
				&tp, 
				sizeof(TOKEN_PRIVILEGES), 
				(PTOKEN_PRIVILEGES) NULL, 
				(PDWORD) NULL) )
			{ 
			  printf("AdjustTokenPrivileges error: %u\n", GetLastError() );
			  return false; 
			} 

			if (GetLastError() == ERROR_NOT_ALL_ASSIGNED)
			{	
				printf("The token does not have the specified privilege. \n");
				return false;
			} 
		}
		CloseHandle(hToken);
		return true;

	}

	/* Get the debug privilege so that we can get process information for all process
	   except for the system process */
	bool obtainDebugPrivilege()
	{
		HANDLE hToken;
		if(OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,&hToken))
		{
			TOKEN_PRIVILEGES tp;
			LUID luid;
			LPCTSTR lpszPrivilege = L"SeDebugPrivilege";

			if ( !LookupPrivilegeValue( 
					NULL,            // lookup privilege on local system
					lpszPrivilege,   // privilege to lookup 
					&luid ) )        // receives LUID of privilege
			{
				printf("LookupPrivilegeValue error: %u\n", GetLastError() ); 
				return false; 
			}

			tp.PrivilegeCount = 1;
			tp.Privileges[0].Luid = luid;
			tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

			// Enable the privilege or disable all privileges.
			if ( !AdjustTokenPrivileges(
				hToken, 
				FALSE, 
				&tp, 
				sizeof(TOKEN_PRIVILEGES), 
				(PTOKEN_PRIVILEGES) NULL, 
				(PDWORD) NULL) )
			{ 
			  printf("AdjustTokenPrivileges error: %u\n", GetLastError() );
			  return false; 
			} 

			if (GetLastError() == ERROR_NOT_ALL_ASSIGNED)
			{	
				printf("The token does not have the specified privilege. \n");
				return false;
			} 
		}
		CloseHandle(hToken);
		return true;
	}

	HANDLE hStopRunning;
private:
	Server* server;
	Visitor* visitor;
	Analyzer* analyzer;

	boost::signals::connection onServerConnectEventConnection;
	boost::signals::connection onServerPingEventConnection;
	
};

int _tmain(int argc, WCHAR* argv[])
{
	/* Set the current directory to the where the CaptureClient.exe is found */
	/* This is a bug fix for the VIX library as the runProgramInGuest function
	   opens the client from within c:\windows\system32 so nothing will load
	   properly during execution */
	wchar_t* szFullPath = new wchar_t[4096];
	GetFullPathName(argv[0], 4096, szFullPath, NULL);
	DebugPrint(L"Capture: Argv[0] -> %s\n", argv[0]);
	DebugPrint(L"Capture: GetFullPathName -> %ls\n", szFullPath);
	wstring dir = szFullPath;
	dir = dir.substr(0, dir.find_last_of(L"\\")+1);
	SetCurrentDirectory(dir.c_str());
	DebugPrint(L"Capture: Dir -> %ls\n", dir.c_str());
	GetCurrentDirectory(4096, szFullPath);

	DebugPrint(L"Capture: Current directory set -> %ls\n", szFullPath);
	
	/* Delete the log directory */
	/*
	GetFullPathName(L"logs", 4096, szFullPath, NULL);
	SHFILEOPSTRUCT deleteLogDirectory;
	deleteLogDirectory.wFunc = FO_DELETE;
	deleteLogDirectory.pFrom = szFullPath;
	deleteLogDirectory.fFlags = FOF_NOCONFIRMATION | FOF_SILENT | FOF_NOERRORUI;
	SHFileOperation(&deleteLogDirectory);
*/
	delete [] szFullPath;
	
	wstring serverIp = L"";
	wstring vmServerId = L"";
	wstring vmId = L"";
	wstring logSystemEventsFile = L"";
	wstring collectModifiedFiles = L"false";
	wstring captureNetworkPackets = L"false";

	for(int i = 1; i < argc; i++)
	{
		wstring option = argv[i];
		if(option == L"--help" || option == L"-h") {
			printf("\nCapture client is a high interaction client honeypot which using event monitoring can monitor the state of the system it is being run on. ");
			printf("Capture can monitor processes, files, and the registry at the moment and classifies an event as being malicious by checking exclusion lists. ");
			printf("\nThese exclusion lists are regular expressions which can either allow or deny a particular event from a process in the system. Because of the fact ");
			printf("that it uses regular expressions, creating these list can be very simple but can also provide very fine grained exclusion if needed.\nThe client ");
			printf("can also copy all modified and deleted files to a temporary directory as well as capture all incoming and outgoing packets on the network adapters ");
			printf("on the system.\nThe client can be run in the following two modes:\n");
			printf("\n\nClient<->Server\n\n");
			printf("The client connects to a central Capture server where the client can be sent urls to visit. These urls can contain other information which tell ");
			printf("the client which program to use, for example www.google.com could be visited with either Internet Explorer or Mozilla Firefox and how long to visit the ");
			printf("url. While the url is being visited the state of the system is monitored and if anything malicious occurs during the visitation period the event ");
			printf("is passed back to the server. In this mode the client is run inside of a virtual machine so that when a malicious event occurs the server can revert ");
			printf("the virtual machine to a clean state\n\n");
			printf("To use this mode the options -s, -a, -b must be set so the client knows how to connect and authenticate with the server. To configure the ");
			printf("programs that are available to visit a url edit the clients.conf file. If -cn are set then the log directory will be sent to the server once ");
			printf("a malicious url is visited. For this to work tar.exe and gzip.exe must be included in the Capture Client folder.\n");
			printf("\n\nStandalone\n\n");
			printf("In this mode the client runs in the background of a system and monitors the state of it. Rather than being controlled by a remote server, in standalone mode ");
			printf("the user has control over the system. Malicious events can either be stored in a file or outputted to stdout.");
			printf("\n\nUsage: CaptureClient.exe [-chn] [-s server address -a vm server id -b vm id] [-l file]\n");
			printf("\n  -h\t\tPrint this help message");
			printf("\n  -s address\tAddress of the server the client connects up to. NOTE -a & -b\n\t\tmust be defined when using this option");
			printf("\n  -a server id\tUnique id of the virtual machine server that hosts the client");
			printf("\n  -b vm id\tUnique id of the virtual machine that this client is run on");
			printf("\n  -l file\tOutput system events to a file rather than stdout\n");
			printf("\n  -c\t\tCopy files into the log directory when they are modified or\n\t\tdeleted");
			printf("\n  -n\t\tCapture all incoming and outgoing network packets from the\n\t\tnetwork adapters on the system and store them in .pcap files in\n\t\tthe log directory");
			printf("\n\nIf -s is not set the client will operate in standalone mode");
			exit(1);
		} else if(option == L"-s") {
			if((i+1 < argc) && (argv[i+1][0] != '-')) {
				serverIp = argv[++i];
				printf("Option: Connect to server: %ls\n", serverIp.c_str());
			}
		} else if(option == L"-a") {
			if((i+1 < argc) && (argv[i+1][0] != '-')) {
				vmServerId = argv[++i];
			}
		} else if(option == L"-b") {
			if((i+1 < argc) && (argv[i+1][0] != '-')) {
				vmId = argv[++i];
			}
		} else if(option == L"-l") {
			if((i+1 < argc) && (argv[i+1][0] != '-')) {			
				logSystemEventsFile = argv[++i];
				printf("Option: Logging system events to %ls\n", logSystemEventsFile.c_str());
			}
		} else {
			if(argv[i][0] == '-')
			{
				for(UINT k = 1; k < option.length(); k++)
				{
					if(argv[i][k] == 'c') {
						printf("Option: Collecting modified files\n");
						collectModifiedFiles = L"true";
					} else if(argv[i][k] == 'n') {
						HMODULE hModWinPcap = LoadLibrary(L"wpcap.dll");
						if(hModWinPcap == NULL)
						{
							printf("NetworkPacketDumper: ERROR - wpcap.dll not found. Check that winpcap is installed on this system\n");
							printf("Cannot use -n option if winpcap is not installed ... exiting\n");
							exit(1);
						} else {
							printf("Option: Capturing network packets\n");
							captureNetworkPackets = L"true";
							FreeLibrary(hModWinPcap);
						}
					}
				}
			}
		}
	}

	OptionsManager::getInstance()->addOption(L"server", serverIp);
	OptionsManager::getInstance()->addOption(L"vm-server-id", vmServerId);
	OptionsManager::getInstance()->addOption(L"vm-id", vmId);
	OptionsManager::getInstance()->addOption(L"log-system-events-file", logSystemEventsFile);
	OptionsManager::getInstance()->addOption(L"collect-modified-files", collectModifiedFiles);
	OptionsManager::getInstance()->addOption(L"capture-network-packets", captureNetworkPackets);

	if(collectModifiedFiles == L"true")
	{
		if(serverIp != L"")
		{
			WIN32_FIND_DATA FindFileData;
			HANDLE hFind;

			hFind = FindFirstFile(L"7za.exe",&FindFileData);
			if(hFind == INVALID_HANDLE_VALUE)
			{
				printf("Analyzer: ERROR - Could not find 7za.exe (http://www.7-zip.org/) - Not collecting modified files\n");
				printf("Cannot use -c option if 7za.exe is not in the current directory ... exiting\n");
				exit(1);
			}
		}
	}

	CaptureClient* cp = new CaptureClient();
	WaitForSingleObject(cp->hStopRunning, INFINITE);
	delete cp;
	return 0;
}
