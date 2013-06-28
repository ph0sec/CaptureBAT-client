#include "ServerReceive.h"

ServerReceive::ServerReceive(Server* s)
{
	server = s;
	running = false;

}

ServerReceive::~ServerReceive(void)
{
	stop();
}

void
ServerReceive::start()
{
	if(!running)
	{
		receiver = new Thread(this);
		receiver->start("ServerReceive");
		running = true;
	}
}

void
ServerReceive::stop()
{
	if(running)
	{
		receiver->stop();
	}
	running = false;
}

void
ServerReceive::run()
{
	//while(server->isConnected())
	//{
		/*
		int bytesRecv = 0;
		char buffer[MAX_RECEIVE_BUFFER];
		wchar_t message[MAX_RECEIVE_BUFFER];
		ZeroMemory(&buffer, MAX_RECEIVE_BUFFER);
		bytesRecv = recv(server->serverSocket, buffer, MAX_RECEIVE_BUFFER, 0);
		if(bytesRecv < 2)
		{
			server->setConnected(false);
			break;
		}
		
		size_t convertedChars = 0;
		mbstowcs_s(&convertedChars, message, strlen(buffer) + 1, buffer, _TRUNCATE);
		printf("Received: %i - > %ls\n",bytesRecv, message);
		*/

		char buffer[MAX_RECEIVE_BUFFER];
		string xmlDocument = "";
		int bytesRecv = 0;
		while((bytesRecv = recv(server->serverSocket, buffer, MAX_RECEIVE_BUFFER, 0)) > 2)
		{
			for(int i = 0; i < bytesRecv; i++)
			{
				if(buffer[i] != '\0')
				{
					xmlDocument += buffer[i];
				} else {
					printf("Got: %s\n", xmlDocument.c_str());
					DebugPrint(L"Capture-ServerReceiver: Received document - length: %i\n", xmlDocument.length());
					EventController::getInstance()->receiveServerEvent(xmlDocument.c_str());
					xmlDocument = "";
				}
			}
			Sleep(100);
			//EventController::getInstance()->receiveServerEvent(message);
		}

		// Received something so pass it onto the event manager
		
	//}
	running = false;
}
