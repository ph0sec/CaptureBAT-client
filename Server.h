/*
 *	PROJECT: Capture
 *	FILE: Server.h
 *	AUTHORS: Ramon Steenson (rsteenson@gmail.com) & Christian Seifert (christian.seifert@gmail.com)
 *
 *	Developed by Victoria University of Wellington and the New Zealand Honeynet Alliance
 *
 *	This file is part of Capture.
 *
 *	Capture is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Capture is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Capture; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */
#ifndef SERVER_H_
#define SERVER_H_

#define MAX_SEND_BUFFER 2048
#define MAX_RECEIVE_BUFFER 2048

#include "CaptureGlobal.h"
#include <string>
#include <queue>
#include <winsock2.h>
#include <Ws2tcpip.h>
#include <boost/signal.hpp>
#include <boost/bind.hpp>
#include <stdlib.h>

using namespace std;
/*
	Class: Server
	
	Manages the server connection. Allows objects to send data or XML to the server.

	When an object wants to send data the data is converted to a multibyte string
	as most events are UNICODE. This data is then queued for sending. In the background
	the server starts 2 thread both <ServerSend> and <ServerReceive>. ServerSend is
	responsible to waiting for data to be queued for sending and sending the actual
	data. ServerReceive receives data from the server, converts it a a wide string
	aka UNICODE and passes the data onto the <EventController> which will dispath the
	event to the objects which a listening for that particular event.

	The Server manages its connection in that it will inform object who listen for
	the onConnectionStatusChanged slot of a change in the connection state of the
	server. This is so that if the connection closes for some reason the client
	can exit gracefully and free all allocated memory.
*/
class Server
{
	friend class ServerSend;
	friend class ServerReceive;
public:
	typedef boost::signal<void (bool)> signal_setConnected;
public:
	Server(wstring serverAddress, int port);
	~Server();
	bool connectToServer(bool startSenderAndReciever = true);
	void sendMessage(wstring* message);
	void sendData(char* data, size_t length);
	void sendXML(wstring elementName, queue<Attribute>* vAttributes);
	void sendXMLElement(Element* pElement);
	//int send(const char* buf,int len,int flags);

	void disconnectFromServer();
	bool isConnected();


	wstring getServerAddress() { return serverAddress; }
	
	
	boost::signals::connection onConnectionStatusChanged(const signal_setConnected::slot_type& s);
private:
	bool initialiseWinsock2();
	void setConnected(bool connected);
	void xml_escape(wstring* xml);

	int port;
	SOCKET serverSocket;
	bool connected;
	wstring serverAddress;
	CRITICAL_SECTION sendQueueLock;
	queue<wstring> sendQueue;
	ServerSend* serverSend;
	ServerReceive* serverReceive;
	
	signal_setConnected signalSetConnected;
protected:
	SOCKET getSocket() { return serverSocket; }
};

#endif /*SERVER_H_*/
