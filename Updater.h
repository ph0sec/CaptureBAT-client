/*
 *	PROJECT: Capture
 *	FILE: Updater.h
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
#pragma once
#include "CaptureGlobal.h"
#include "Server.h"
#include <string.h>
#include <boost/lexical_cast.hpp>

/*
	Class: Updater

	NOTE only used in client server mode

	Receives client updates from the server. Exits the client and runs the updater.

	Very similar to <FileDownloader>

	The update will then relaunch the client, inform the server of the update. The
	server will then probably save the new state of the VM this client is hosted on
	and rerun the client again.
*/
using namespace std;

class Updater
{
public:
	Updater(Server* s);
	~Updater(void);

private:

	void onReceiveUpdateEvent(Element* pElement);
	void onReceiveUpdatePartEvent(Element* pElement);
	void onReceiveUpdateFinishedEvent(Element* pElement);

	boost::signals::connection onReceiveUpdateEventConnection;
	boost::signals::connection onReceiveUpdatePartEventConnection;
	boost::signals::connection onReceiveUpdateFinishedEventConnection;

	bool fileAllocated;
	char* fileBuffer;
	int fileSize;
	wstring fileName;

	Server* server;
};