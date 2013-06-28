#include "Monitor.h"

Monitor::Monitor()
{
	hService = NULL;
}

Monitor::~Monitor()
{
	stdext::hash_map<wstring, std::list<Permission*>*>::iterator it;
	for(it = permissionMap.begin(); it != permissionMap.end(); it++)
	{
		std::list<Permission*>::iterator lit;
		for(lit = it->second->begin(); lit != it->second->end(); lit++)
		{
			delete (*lit);
		}
		it->second->clear();
	}
	permissionMap.clear();
	unInstallKernelDriver();
}

bool
Monitor::isEventAllowed(std::wstring eventType, std::wstring subject, std::wstring object)
{
	stdext::hash_map<wstring, std::list<Permission*>*>::iterator it;
	std::transform(eventType.begin(),eventType.end(),eventType.begin(),std::towlower);
	it = permissionMap.find(eventType);
	PERMISSION_CLASSIFICATION excluded = NO_MATCH;
	if(it != permissionMap.end())
	{
		std::list<Permission*>* lp = it->second;
		std::list<Permission*>::iterator lit;

		for(lit = lp->begin(); lit != lp->end(); lit++)
		{
			PERMISSION_CLASSIFICATION newExcluded = (*lit)->Check(subject,object);
			if( newExcluded == ALLOWED)
			{
				if(excluded != DISALLOWED)
					excluded = ALLOWED;
			} else if(newExcluded == DISALLOWED) {
				excluded = DISALLOWED;
			}
		}
	}
	if(excluded == ALLOWED)
	{
		return true;
	} else {
		return false;
	}
}

void
Monitor::clearExclusionList()
{
	stdext::hash_map<wstring, std::list<Permission*>*>::iterator it;
	for(it = permissionMap.begin(); it != permissionMap.end(); it++)
	{
		std::list<Permission*>* lp = it->second;
		std::list<Permission*>::iterator lit;
		for(lit = lp->begin(); lit != lp->end(); lit++)
		{
			Permission* p = *lit;
			if(!p->permaneant)
			{
				lp->remove(p);
				delete (p);
			}
		}
	}
}

bool
Monitor::installKernelDriver(wstring driverPath, wstring driverName, wstring driverDescription)
{
	SC_HANDLE hSCManager;

    hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);

    if(hSCManager)
    {
		//printf("%ls: Kernel driver path: %ls\n", driverName.c_str(), driverPath.c_str());
        hService = CreateService(hSCManager, driverName.c_str(), 
								  driverDescription.c_str(), 
                                  SERVICE_START | DELETE | SERVICE_STOP, 
                                  SERVICE_KERNEL_DRIVER,
                                  SERVICE_DEMAND_START, 
                                  SERVICE_ERROR_IGNORE, 
								  driverPath.c_str(), 
                                  NULL, NULL, NULL, NULL, NULL);

        if(!hService)
        {
			hService = OpenService(hSCManager, driverName.c_str(), 
                       SERVICE_START | DELETE | SERVICE_STOP);
        }

        if(hService)
        {
            if(StartService(hService, 0, NULL))
			{
				printf("Loaded kernel driver: %ls\n", driverName.c_str());
			} else {
				DWORD err = GetLastError();
				if(err == ERROR_SERVICE_ALREADY_RUNNING)
				{
					printf("Driver already loaded: %ls\n", driverName.c_str());
				} else {
					printf("Error loading kernel driver: %ls - 0x%08x\n", driverName.c_str(), err);
					CloseServiceHandle(hSCManager);
					return false;
				}
			}
		} else {
			printf("Error loading kernel driver: %ls - 0x%08x\n", driverName.c_str(), GetLastError());
			CloseServiceHandle(hSCManager);
			return false;
		}
        CloseServiceHandle(hSCManager);
		return true;
    }
	printf("Error loading kernel driver: %ls - OpenSCManager 0x%08x\n", driverName.c_str(), GetLastError());
	return false;
}

void
Monitor::unInstallKernelDriver()
{
	if(hService != NULL)
	{
		SERVICE_STATUS ss;
		ControlService(hService, SERVICE_CONTROL_STOP, &ss);
		DeleteService(hService);
		CloseServiceHandle(hService);
	}
	hService = NULL;
}

void
Monitor::loadExclusionList(wstring file)
{
	string line;
	int lineNumber = 0;
	DebugPrint(L"Monitor-loadExclusionList: Loading list - %ls\n", file.c_str());
	ifstream exclusionList (file.c_str());
	if (exclusionList.is_open())
	{
		while (! exclusionList.eof() )
		{
			getline (exclusionList,line);
			lineNumber++;
			if(line.length() > 0 && line.at(0) != '#') {
				
				try {
					if(line.at(0) == '+' || line.at(0) == '-')
					{
						vector<std::wstring> splitLine;

						typedef split_iterator<string::iterator> sf_it;
						for(sf_it it=make_split_iterator(line, token_finder(is_any_of("\t")));
							it!=sf_it(); ++it)
						{
							splitLine.push_back(copy_range<std::wstring>(*it));				
						}

						if(splitLine.size() == 4)
						{
							if(splitLine[1] == L".*" || splitLine[1] == L".+")
							{
								printf("%ls ERROR on line %i: The action type is not supposed to be a regular expression\n", file.c_str(), lineNumber);
							} else {
								addExclusion(splitLine[0], splitLine[1], splitLine[2], splitLine[3]);
							}
						} else {
							printf("%ls token ERROR on line %i\n", file.c_str(), lineNumber);
						}
					} else {
						printf("%ls ERROR no exclusion type (+,-) on line %i\n", file.c_str(), lineNumber);
					}
				} catch(boost::regex_error r) {				
					printf("%ls ERROR on line %i\n", file.c_str(), lineNumber);
					printf("\t%s\n", r.what());
				}
			}
		}
	} else {
		printf("Could not open file: %ls\n", file.c_str());
	}
}

void
Monitor::prepareStringForExclusion(wstring* s)
{
	wstring from = L"\\";
	wstring to = L"\\\\";
	size_t offset = 0;
	while((offset = s->find(from, offset)) != wstring::npos)
	{
		s->replace(offset, 
			from.size(), 
			to);
		offset += to.length();
	}
	from = L".";
	to = L"\\.";
	offset = 0;
	while((offset = s->find(from, offset)) != wstring::npos)
	{
		s->replace(offset, 
			from.size(), 
			to);
		offset += to.length();
	}
}

void
Monitor::addExclusion(wstring excluded, wstring action, wstring subject, wstring object , bool permaneant)
{
	//printf("Adding exclusion\n");
	try {
		Permission* p = new Permission();
		if(excluded == L"yes" || excluded == L"+")
		{
			p->allow = true;
		} else if(excluded == L"no" || excluded == L"-"){
			p->allow = false;
		}
		p->permaneant = permaneant;
		boost::wregex subjectRegex(subject.c_str(), boost::wregex::icase);
		boost::wregex objectRegex(object.c_str(), boost::wregex::icase);
		p->objects.push_back(objectRegex);
		p->subjects.push_back(subjectRegex);
		std::transform(action.begin(),action.end(),action.begin(),std::towlower);
		stdext::hash_map<wstring, std::list<Permission*>*>::iterator it;
		it = permissionMap.find(action);

		if(it == permissionMap.end())
		{
			std::list<Permission*>*l = new list<Permission*>();
			l->push_back(p);
			permissionMap.insert(Permission_Pair(action, l));
		} else {
			std::list<Permission*>* lp = it->second;
			lp->push_back(p);
		}
	} catch(boost::regex_error r) {
		throw r;
	}
}
