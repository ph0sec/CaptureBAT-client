/*
 *	PROJECT: Capture
 *	FILE: FileUploader.h
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
#include <boost/lexical_cast.hpp>

/*
	Class: FileUploader

	Uploads files to the remote server. Encodes binary data into base64 which is then
	split up into XML documents to be sent to the server.

	Listens to various events in the <EventController> to do with progress of the file
	transfer.
*/
class FileUploader
{
public:
	FileUploader(Server* s);
	~FileUploader(void);

	bool sendFile(wstring file);
private:
	Server* server;

	void onReceiveFileOkEvent(Element* pElement);
	void onReceiveFileErrorEvent(Element* pElement);
	void onReceiveFileAcceptEvent(Element* pElement);
	void onReceiveFileRejectEvent(Element* pElement);

	BOOL getFileSize(wstring file, PLARGE_INTEGER fileSize);
	size_t sendFilePart(unsigned int start, unsigned int size);

	boost::signals::connection onReceiveFileOkEventConnection;
	boost::signals::connection onReceiveFileErrorEventConnection;
	boost::signals::connection onReceiveFileAcceptEventConnection;
	boost::signals::connection onReceiveFileRejectEventConnection;

	bool fileAccepted;
	bool fileOpened;
	wstring fileName;
	bool busy;
	LARGE_INTEGER fileSize;
	HANDLE hFileAcknowledged;
	FILE* pFileStream;
};
