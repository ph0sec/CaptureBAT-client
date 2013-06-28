#include "FileUploader.h"

FileUploader::FileUploader(Server* s)
{
	server = s;
	hFileAcknowledged = CreateEvent(NULL, FALSE, FALSE, NULL);
	fileAccepted = false;
	fileOpened = false;
	busy = false;

	onReceiveFileOkEventConnection = EventController::getInstance()->connect_onServerEvent(L"file-ok", boost::bind(&FileUploader::onReceiveFileOkEvent, this, _1));
	onReceiveFileErrorEventConnection = EventController::getInstance()->connect_onServerEvent(L"file-error", boost::bind(&FileUploader::onReceiveFileErrorEvent, this, _1));
	onReceiveFileAcceptEventConnection = EventController::getInstance()->connect_onServerEvent(L"file-accept", boost::bind(&FileUploader::onReceiveFileAcceptEvent, this, _1));
	onReceiveFileRejectEventConnection = EventController::getInstance()->connect_onServerEvent(L"file-reject", boost::bind(&FileUploader::onReceiveFileRejectEvent, this, _1));
}

FileUploader::~FileUploader(void)
{
	onReceiveFileOkEventConnection.disconnect();
	onReceiveFileErrorEventConnection.disconnect();
	onReceiveFileAcceptEventConnection.disconnect();
	onReceiveFileRejectEventConnection.disconnect();

	if(fileOpened)
	{
		fclose(pFileStream);
	}
}

void
FileUploader::onReceiveFileOkEvent(Element* pElement)
{

}

void
FileUploader::onReceiveFileErrorEvent(Element* pElement)
{
	int start = 0;
	int end = 0;
	vector<Attribute>::iterator it;
	for(it = pElement->attributes.begin(); it != pElement->attributes.end(); it++)
	{
		if(it->name == L"part-start") {
			start = boost::lexical_cast<int>(it->value);
		} else if(it->name == L"part-end") {
			end = boost::lexical_cast<int>(it->value);
		}
	}

	/* If the part-start and part-end attributes are set then send that part again */
	if((start != 0) && (end != 0))
	{
		int size = end - start;
		sendFilePart(start, end);
	} else {
		/* TODO General error */
	}
}

void
FileUploader::onReceiveFileAcceptEvent(Element* pElement)
{
	fileAccepted = true;
	SetEvent(hFileAcknowledged);
}

void
FileUploader::onReceiveFileRejectEvent(Element* pElement)
{
	SetEvent(hFileAcknowledged);
}

BOOL
FileUploader::getFileSize(wstring file, PLARGE_INTEGER fileSize)
{
	/* Open the file and get is size */
	HANDLE hFile;
	hFile = CreateFile(file.c_str(),    // file to open
                   GENERIC_READ,          // open for reading
                   FILE_SHARE_READ,       // share for reading
                   NULL,                  // default security
                   OPEN_EXISTING,         // existing file only
                   FILE_ATTRIBUTE_NORMAL, // normal file
                   NULL);                 // no attr. template
	if(hFile ==  INVALID_HANDLE_VALUE)
	{
		printf("FileUploader: ERROR - Could get file size: %08x\n", GetLastError());
		return false;
	} else {
		BOOL gotSize = FALSE;
		gotSize = GetFileSizeEx(hFile, fileSize);
		CloseHandle(hFile);
		return gotSize;
	}
}

bool
FileUploader::sendFile(wstring file)
{
	if(!server->isConnected())
	{
		printf("FileUploader: ERROR - Not connected to server so not sending file\n");
		return false;
	}

	busy = true;

	Attribute fileNameAttribute;
	Attribute fileSizeAttribute;
	Attribute fileTypeAttribute;
	fileName = file;
	errno_t error;
	

	/* Get the file size */
	if(!getFileSize(file, &fileSize))
	{
		busy = false;
		return false;
	}

	error = _wfopen_s( &pFileStream, file.c_str(), L"rb");
	if(error != 0)
	{
		printf("FileUploader: ERROR - Could not open file: %08x\n", error);
		busy = false;
		return false;
	} else {
		fileOpened = true;
	}
	
	queue<Attribute> atts;
	fileNameAttribute.name = L"name";
	fileNameAttribute.value = file;
	fileSizeAttribute.name = L"size";
	fileSizeAttribute.value = boost::lexical_cast<wstring>(fileSize.QuadPart);
	fileTypeAttribute.name = L"type";
	fileTypeAttribute.value = file.substr(file.find_last_of(L".")+1);
	atts.push(fileNameAttribute);
	atts.push(fileSizeAttribute);
	atts.push(fileTypeAttribute);
	
	/* Send request to server to accept a file */
	server->sendXML(L"file", &atts);

	/* Wait for the server to accept the file or timeout if it fails 
	   to respond or rejects it */
	
	DWORD timeout = WaitForSingleObject(hFileAcknowledged, 5000);

	if((timeout == WAIT_TIMEOUT) || !fileAccepted)
	{
		printf("FileUploader: ERROR - Server did not acknowledge the request to receive a file\n");
		busy = false;
		if(fileOpened)
		{
			fclose(pFileStream);
			fileOpened = false;
		}
		return false;	
	}	
 
	printf("FileUplodaer: Sending file: %ls\n", fileName.c_str());

	/* Loop sending the file inside <file-part /> xml messages */
	size_t offset = 0;	
	size_t bytesRead;
	do
	{
		bytesRead = sendFilePart((UINT)offset, 8192);
		offset += bytesRead;
	} while(bytesRead > 0);

	atts.push(fileNameAttribute);
	atts.push(fileSizeAttribute);

	server->sendXML(L"file-finished", &atts);

	fclose(pFileStream);
	fileOpened = false;
	busy = false;
	return true;
}

size_t
FileUploader::sendFilePart(unsigned int offset, unsigned int size)
{
	if(fileOpened)
	{
		char *pFilePart = (char*)malloc(size);
		Element* pElement = new Element();
		pElement->name = L"part";
	
		Attribute fileNameAttribute;
		Attribute filePartStartAttribute;
		Attribute filePartEndAttribute;
		Attribute filePartEncoding;

		fileNameAttribute.name = L"name";
		fileNameAttribute.value = fileName.c_str();
		filePartStartAttribute.name = L"part-start";
		filePartStartAttribute.value = boost::lexical_cast<wstring>(offset);
		filePartEncoding.name = L"encoding";
		filePartEncoding.value = L"base64";

		pElement->attributes.push_back(fileNameAttribute);
		pElement->attributes.push_back(filePartStartAttribute);
		pElement->attributes.push_back(filePartEncoding);
		

		/* Seek to offset and read size bytes */
		//printf("size: %i\n", size);
		//printf("offset: %i\n", offset);
		int err = fseek(pFileStream, offset, 0);
		//printf("fseel: %i\n", err);
		size_t bytesRead = fread(pFilePart , sizeof(char), size, pFileStream);
		//printf("bytesRead: %i\n", bytesRead);
		//printf("fread error: %i\n", ferror(pFileStream));
		filePartEndAttribute.name = L"part-end";
		int end = offset + bytesRead;
		filePartEndAttribute.value = boost::lexical_cast<wstring>(end);
		pElement->attributes.push_back(filePartEndAttribute);

		/* Encode the data just read in base64 and append to XML element */
		size_t encodedLength = 0;
		char* pEncodedFilePart = encode_base64(pFilePart, bytesRead, &encodedLength);
		pElement->data = pEncodedFilePart;
		pElement->dataLength = encodedLength;

		/* Send the XML element to the server */
		//printf("feof: %i\n", feof(pFileStream));
		if(bytesRead > 0)
		{
			server->sendXMLElement(pElement);
		}

		/* Cleanup */
		free(pEncodedFilePart);
		free(pFilePart);
		delete pElement;
		if(feof(pFileStream) > 0)
		{
			return 0;
		} else {
			return bytesRead;
		}
	}
	return 0;
}
