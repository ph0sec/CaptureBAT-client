#include "EventController.h"

EventController::EventController(void)
{
	pCurrentElement = NULL;
}

EventController::~EventController(void)
{
	instanceCreated = false;
	if(pCurrentElement != NULL)
	{
		if(pCurrentElement->data != NULL)
		{
			free(pCurrentElement->data);
		}
		delete pCurrentElement;
		pCurrentElement = NULL;
	}
}

EventController*
EventController::getInstance()
{
	if(!instanceCreated)
	{
		pEventController = new EventController();
		instanceCreated = true;	
	}
	return pEventController;
}

boost::signals::connection 
EventController::connect_onServerEvent(wstring eventType, 
										  const signal_serverEvent::slot_type& s)
{
	stdext::hash_map<wstring, signal_serverEvent*>::iterator it;
	if((it = onServerEventMap.find(eventType)) != onServerEventMap.end())
	{
		signal_serverEvent* signal_onServerEvent = it->second;
		return signal_onServerEvent->connect(s);
	} else {
		signal_serverEvent* signal_onServerEvent = new signal_serverEvent();
		boost::signals::connection connection = signal_onServerEvent->connect(s);
		onServerEventMap.insert(OnServerEventPair(eventType, signal_onServerEvent)); 
		return connection;
	}
}

void
EventController::notifyListeners()
{
	if(pCurrentElement != NULL)
	{
		stdext::hash_map<wstring, signal_serverEvent*>::iterator it;
		it = onServerEventMap.find(pCurrentElement->name);
		if(it != onServerEventMap.end())
		{
			signal_serverEvent* signal_onServerEvent = it->second;
			(*signal_onServerEvent)(pCurrentElement);
		}	
		if(pCurrentElement->data != NULL)
		{
			free(pCurrentElement->data);
		}
		delete pCurrentElement;
		pCurrentElement = NULL;
	}
}

void 
EventController::receiveServerEvent(const char* xmlDocument)
{
	this->parseString(xmlDocument);
}

void 
EventController::startElement(const char* name, const char** atts)
{
	size_t nameLength = strlen(name) + 1;
	wchar_t* wszElementName = (wchar_t*)malloc((strlen(name) + 1)*sizeof(wchar_t));
	size_t convertedChars = 0;
	mbstowcs_s(&convertedChars, wszElementName, nameLength, name, _TRUNCATE);
	wstring elementName = wszElementName;
	free(wszElementName);

	if(pCurrentElement != NULL)
	{
		notifyListeners();
	}

	pCurrentElement = new Element();
	pCurrentElement->name = elementName;
	pCurrentElement->data = NULL;

	if (atts)
	{
		for(int i = 0; atts[i]; i+=2)
		{
			if(atts[i+1] != NULL)
			{
				Attribute att;

				size_t attributeNameLength = strlen(atts[i]) + 1;
				size_t attributeValueLength = strlen(atts[i+1]) + 1;
				wchar_t* wszAttributeName = (wchar_t*)malloc(attributeNameLength*sizeof(wchar_t));
				wchar_t* wszAttributeValue = (wchar_t*)malloc(attributeValueLength*sizeof(wchar_t));
			
				mbstowcs_s(&convertedChars, wszAttributeName, attributeNameLength, atts[i], _TRUNCATE);
				mbstowcs_s(&convertedChars, wszAttributeValue, attributeValueLength, atts[i+1], _TRUNCATE);
				
				att.name = wszAttributeName;
				att.value = wszAttributeValue;

				pCurrentElement->attributes.push_back(att);

				free(wszAttributeName);
				free(wszAttributeValue);

			} else {
				printf("EventController: ERROR - Malformed XML received\n");
			}
		}
	}
}


void 
EventController::endElement(const XML_Char* name)
{
	notifyListeners();
}


void 
EventController::charData(const XML_Char *s, int len)
{
	char* data = (char*)malloc(len+1);
	memcpy(data, &s[0], len);
	data[len] = '\0';
	pCurrentElement->data = data;
	pCurrentElement->dataLength = len;
}
