#include "Updater.h"

Updater::Updater(Server* s)
{
	server = s;

	onReceiveUpdateEventConnection = EventController::getInstance()->connect_onServerEvent(L"update", boost::bind(&Updater::onReceiveUpdateEvent, this, _1));
	onReceiveUpdatePartEventConnection = EventController::getInstance()->connect_onServerEvent(L"update-part", boost::bind(&Updater::onReceiveUpdatePartEvent, this, _1));
	onReceiveUpdateFinishedEventConnection = EventController::getInstance()->connect_onServerEvent(L"update-finished", boost::bind(&Updater::onReceiveUpdateFinishedEvent, this, _1));
}

Updater::~Updater(void)
{
	if(fileAllocated)
	{
		free(fileBuffer);
	}
	onReceiveUpdateEventConnection.disconnect();
	onReceiveUpdatePartEventConnection.disconnect();
	onReceiveUpdateFinishedEventConnection.disconnect();
}


void
Updater::onReceiveUpdateEvent(Element* pElement)
{
	if(!fileAllocated)
	{
		vector<Attribute>::iterator it;
		for(it = pElement->attributes.begin(); it != pElement->attributes.end(); it++)
		{
			if(it->name == L"file-size") {
				fileSize = boost::lexical_cast<int>(it->value);
			} else if(it->name == L"file-name") {
				fileName = it->value;
			}
		}
		fileBuffer = (char*)malloc(fileSize);
		fileAllocated = true;
		pElement->name = L"update-accept";
		server->sendXMLElement(pElement);
	} else {
		printf("Updater: ERROR - Can only download one update at a time\n");
		pElement->name = L"update-reject";
		server->sendXMLElement(pElement);
	}
}

void
Updater::onReceiveUpdatePartEvent(Element* pElement)
{
	vector<Attribute>::iterator it;
	int start = 0;
	int end = 0;
	int partSize = 0;
	for(it = pElement->attributes.begin(); it != pElement->attributes.end(); it++)
	{
		if(it->name == L"file-part-start") {
			start = boost::lexical_cast<int>(it->value);
		} else if(it->name == L"file-part-end") {
			end = boost::lexical_cast<int>(it->value);
		}
	}

	if(fileAllocated)
	{
		partSize = end - start;

		// TODO Check the checksum of the part is correct

		decode_base64(pElement->data);

		memcpy((void*)fileBuffer[start], pElement->data, partSize);
	} else {
		printf("Updater: ERROR - File not allocated: %08x\n", GetLastError());
		pElement->name = L"update-error";
		server->sendXMLElement(pElement);
	}
}

void
Updater::onReceiveUpdateFinishedEvent(Element* pElement)
{
	HANDLE hFile;
	int offset = 0;
 
	hFile = CreateFile(fileName.c_str(),    // file to open
                   GENERIC_WRITE,          // open for reading
                   FILE_SHARE_READ,       // share for reading
                   NULL,                  // default security
                   CREATE_ALWAYS,         // existing file only
                   FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS, // normal file
                   NULL);                 // no attr. template
	if(hFile ==  INVALID_HANDLE_VALUE)
	{
		printf("Updater: ERROR - Could not open file: %08x\n", GetLastError());
		pElement->name = L"update-error";
		server->sendXMLElement(pElement);
	} else {
		DWORD bytesWritten = 0;
		WriteFile(hFile, fileBuffer, fileSize, &bytesWritten, NULL);
		pElement->name = L"update-ok";
		server->sendXMLElement(pElement);
		fileAllocated = false;
		free(fileBuffer);
	}
}