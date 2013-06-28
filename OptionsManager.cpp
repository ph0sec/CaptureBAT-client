#include "OptionsManager.h"

OptionsManager::OptionsManager(void)
{
	onOptionEventConnection = EventController::getInstance()->connect_onServerEvent(L"option", boost::bind(&OptionsManager::onOptionEvent, this, _1));
}

OptionsManager::~OptionsManager(void)
{
	optionsMap.clear();
	onOptionEventConnection.disconnect();
	instanceCreated = false;
}

OptionsManager*
OptionsManager::getInstance()
{
	if(!instanceCreated)
	{
		optionsManager = new OptionsManager();
		instanceCreated = true;	
	}
	return optionsManager;
}

void
OptionsManager::onOptionEvent(Element* pElement)
{
	if(pElement->name == L"option")
	{	
		wstring option = L"";
		wstring value = L"";
		vector<Attribute>::iterator it;
		for(it = pElement->attributes.begin(); it != pElement->attributes.end(); it++)
		{
			if(it->name == L"name") {
				option = it->value;
			} else if(it->name == L"value") {
				value = it->value;
			}
		}
		if(option != L"" && value != L"")
		{
			DebugPrint(L"Received option event: %ls => %ls\n", option.c_str(), value.c_str());
			addOption(option, value);
		}
	}
}

wstring
OptionsManager::getOption(wstring option)
{
	stdext::hash_map<wstring, wstring>::iterator it;
	it = optionsMap.find(option);
	if(it != optionsMap.end())
	{
		return it->second;
	}
	return L"";
}

bool
OptionsManager::addOption(wstring option, wstring value)
{
	stdext::hash_map<wstring, wstring>::iterator it;
	it = optionsMap.find(option);
	if(it == optionsMap.end())
	{
		DebugPrint(L"Adding option: %ls => %ls\n", option.c_str(), value.c_str());
		optionsMap.insert(OptionPair(option, value));
	} else {
		DebugPrint(L"Changing option: %ls => %ls\n", option.c_str(), value.c_str());
		it->second = value;
	}
	signalOnOptionChanged(option);
	return true;
}

boost::signals::connection 
OptionsManager::connect_onOptionChanged(const signal_optionChanged::slot_type& s)
{
	boost::signals::connection conn = signalOnOptionChanged.connect(s);
	
	stdext::hash_map<wstring, wstring>::iterator it;
	for(it = optionsMap.begin(); it != optionsMap.end(); it++)
	{
		signalOnOptionChanged(it->first);
	}
	
	return conn;
}
