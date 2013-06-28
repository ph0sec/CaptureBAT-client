/*
 *	PROJECT: Capture
 *	FILE: Logger.h
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
#include <string>
#include <queue>
#include <list>
/*
	Class: Logger

	A singleton which is accessible to all objects. Allows any object to write data
	to the log file if required. The "-l file" option must be specified for data
	to be logged
*/
class Logger
{
public:
	~Logger(void);
	static Logger* getInstance();

	void openLogFile(wstring file);

	void writeToLog(wstring* message);
	void writeSystemEventToLog(wstring* type, wstring* time, wstring* process, wstring* action, wstring* object);

	void closeLogFile();

	bool isFileOpen() { return fileOpen; }

	wstring getLogFileName() { return logFileName; }
	wstring getLogFullPath() { return logFullPath; }

private:
	static bool instanceCreated;
    static Logger *logger;
	Logger(void);

	char* convertToMultiByteString(wstring* message, size_t* charsConverted);

	bool fileOpen;
	wstring logFullPath;
	wstring logFileName;
	HANDLE hLog;
};