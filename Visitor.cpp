#include "Visitor.h"

Visitor::Visitor(void)
{
	visiting = false;

	hQueueNotEmpty = CreateEvent(NULL, FALSE, FALSE, NULL);
	onServerVisitEventConnection=EventController::getInstance()->connect_onServerEvent(L"visit", boost::bind(&Visitor::onServerEvent, this, _1));

	loadClientPlugins();

	visitorThread = new Thread(this);
	visitorThread->start("Visitor");
}

Visitor::~Visitor(void)
{
	onServerVisitEventConnection.disconnect();
	CloseHandle(hQueueNotEmpty);
	unloadClientPlugins();
	// TODO free items in toVisit queue
}

boost::signals::connection 
Visitor::onVisitEvent(const signal_visitEvent::slot_type& s)
{ 
	return signalVisitEvent.connect(s); 
}

void
Visitor::run()
{
	while(true)
	{
		WaitForSingleObject(hQueueNotEmpty, INFINITE);
		VisitPair visit = toVisit.front();
		toVisit.pop();
		DWORD minorErrorCode = 0;
		DWORD majorErrorCode = 0;
		visiting = true;

		ApplicationPlugin* applicationPlugin = visit.first;
		Url* url = visit.second;

		signalVisitEvent(CAPTURE_VISITATION_PRESTART, SUCCESS, url->getUrl(), url->getApplicationName());

		signalVisitEvent(CAPTURE_VISITATION_START, SUCCESS, url->getUrl(), url->getApplicationName());

		printf("Visiting: %ls -> %ls\n", url->getApplicationName().c_str(), url->getUrl().c_str());
		
		/* Pass the actual visitation process of to the application plugin */
		majorErrorCode = applicationPlugin->visitUrl(url, &minorErrorCode);
		
		if(majorErrorCode != CAPTURE_VISITATION_OK)
		{
			signalVisitEvent(majorErrorCode, minorErrorCode, url->getUrl(), url->getApplicationName());
		} else {
			signalVisitEvent(CAPTURE_VISITATION_FINISH, SUCCESS, url->getUrl(), url->getApplicationName());
		}	
		
		signalVisitEvent(CAPTURE_VISITATION_POSTFINISH, SUCCESS, url->getUrl(), url->getApplicationName());

		visiting = false;
	}
}

void
Visitor::loadClientPlugins()
{
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	wchar_t pluginDirectoryPath[1024];

	GetFullPathName(L"plugins\\Application_*.dll", 1024, pluginDirectoryPath, NULL);
	DebugPrint(L"Capture-Visitor: Plugin directory - %ls\n", pluginDirectoryPath);
	hFind = FindFirstFile(pluginDirectoryPath, &FindFileData);

	if (hFind != INVALID_HANDLE_VALUE) 
	{
		typedef void (*AppPlugin)(void*);
		do
		{
			wstring pluginDir = L"plugins\\";
			pluginDir += FindFileData.cFileName;			
			HMODULE hPlugin = LoadLibrary(pluginDir.c_str());

			if(hPlugin != NULL)
			{
				list<ApplicationPlugin*>* apps = new std::list<ApplicationPlugin*>();
				applicationPlugins.insert(PluginPair(hPlugin, apps));
				ApplicationPlugin* applicationPlugin = createApplicationPluginObject(hPlugin);
				if(applicationPlugin == NULL) {
					FreeLibrary(hPlugin);
				} else {
					printf("Loaded plugin: %ls\n", FindFileData.cFileName);
					unsigned int g = applicationPlugin->getPriority();
					wchar_t** supportedApplications = applicationPlugin->getSupportedApplicationNames();
					for(int i = 0; supportedApplications[i] != NULL; i++)
					{
						stdext::hash_map<wstring, ApplicationPlugin*>::iterator it;
						it = applicationMap.find(supportedApplications[i]);
						/* Check he application isn't already being handled by a plugin */
						if(it != applicationMap.end())
						{
							/* Check the priority of the existing application plugin */
							unsigned int p = it->second->getPriority();
							if(applicationPlugin->getPriority() > p)
							{
								/* Over ride the exisiting plugin if the priority of the loaded one
								   is greater */
								applicationMap.erase(supportedApplications[i]);
								printf("\toverride: added application: %ls\n", supportedApplications[i]);
								applicationMap.insert(ApplicationPair(supportedApplications[i], applicationPlugin));
							} else {
								printf("\tplugin overridden: not adding application: %ls\n", supportedApplications[i]);
							}
						} else {
							printf("\tinserted: added application: %ls\n", supportedApplications[i]);
							applicationMap.insert(ApplicationPair(supportedApplications[i], applicationPlugin)); 
						}
					}
				}
			}
		} while(FindNextFile(hFind, &FindFileData) != 0);
		FindClose(hFind);
	}
	
}

ApplicationPlugin*
Visitor::createApplicationPluginObject(HMODULE hPlugin)
{
	typedef void (*PluginExportInterface)(void*);
	PluginExportInterface pluginCreateInstance = NULL;
	ApplicationPlugin* applicationPlugin = NULL;
	/* Get the function address to create a plugin object */
	pluginCreateInstance = (PluginExportInterface)GetProcAddress(hPlugin,"New");
	/* Create a new plugin object in the context of the plugin */
	pluginCreateInstance(&applicationPlugin);
	/* If the object was created then add it to a list so we can track it */
	if(applicationPlugin != NULL)
	{
		stdext::hash_map<HMODULE, std::list<ApplicationPlugin*>*>::iterator it;
		it = applicationPlugins.find(hPlugin);
		if(it != applicationPlugins.end())
		{
			list<ApplicationPlugin*>* apps = it->second;
			apps->push_back(applicationPlugin);
		}
	}
	return applicationPlugin;
}

void
Visitor::unloadClientPlugins()
{
	typedef void (*PluginExportInterface)(void*);
	stdext::hash_map<HMODULE, std::list<ApplicationPlugin*>*>::iterator it;
	for(it = applicationPlugins.begin(); it != applicationPlugins.end(); it++)
	{
		std::list<ApplicationPlugin*>::iterator lit;
		list<ApplicationPlugin*>* apps = it->second;
		PluginExportInterface pluginDeleteInstance = (PluginExportInterface)GetProcAddress(it->first,"Delete");
		for(lit = apps->begin(); lit != apps->end(); lit++)
		{
			pluginDeleteInstance(&(*lit));
		}
		delete apps;
		FreeLibrary(it->first);
	}
}

void
Visitor::onServerEvent(Element* pElement)
{
	wstring applicationName = L"iexplore";
	wstring url = L"";
	int time = 30;
	vector<Attribute>::iterator it;
	for(it = pElement->attributes.begin(); it != pElement->attributes.end(); it++)
	{
		if(it->name == L"url") {
			url = it->value;
		} else if(it->name == L"program") {
			applicationName = it->value;
		} else if(it->name == L"time") {
			time = boost::lexical_cast<int>(it->value);
		}
	}
	if(url != L"")
	{
		url = CaptureGlobal::urlDecode(url);
		stdext::hash_map<wstring, ApplicationPlugin*>::iterator vit;
		vit = applicationMap.find(applicationName);
		if(vit != applicationMap.end())
		{
			ApplicationPlugin* applicationPlugin = vit->second;
			Url* visiturl = new Url(url, applicationName, time);
			toVisit.push(VisitPair(applicationPlugin, visiturl));
			SetEvent(hQueueNotEmpty);
		} else {
			printf("Visitor-onServerEvent: ERROR could not find client %ls path, url not queued for visitation\n", applicationName.c_str());
		}
	} else {
		printf("Visitor-onServerEvent: ERROR no url specified for visit event\n");
	}
}
