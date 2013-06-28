/*
 *	PROJECT: Capture
 *	FILE: Analyzer.h
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
#include "shellapi.h"
#include <boost/signal.hpp>
#include <boost/bind.hpp>
#include "Server.h"
#include "Visitor.h"
#include "ProcessMonitor.h"
#include "RegistryMonitor.h"
#include "FileMonitor.h"
#include "NetworkPacketDumper.h"
#include "FileUploader.h"

using namespace std;

/*
	Class: Analyzer

	The analyzer is the central part of the client component of Capture.

	All malicious events that occur on the system are passed onto the Analyzer,
	where it is formatted to be outputted to the server, stdout, or a file. It
	uses the signal slot design in Boost to achieve a common interface for which
	the monitors can send events to it. These are the on*Event() methods and are
	managed by connecting/binding a particular method in the an Analyzer to a
	particular slot on a monitor. When a malicious event occurs in a monitor,
	the monitor will signal the slot with the event information which is then
	processed by the analyzer.
*/
class Analyzer
{
public:
	Analyzer(Visitor* v, Server* s);
	~Analyzer(void);

	/*
		Function: start

		Connect to all of the available monitor slots so that the Analyzer can
		receive malicious events
	*/
	void start();
	/*
		Function: stop

		Disconnects all of the connections between the monitors slots so that
		the Analyzer does not receive malicious events
	*/
	void stop();
	/*
		Function: onVisitEvent

		Method to bind to the <Visitor> visit event slot. These are called when 
		operating in client server mode. During the visitation of the URL by and 
		application, the Visitor will signal various visit events which will be 
		passed onto the Analyzer.
	*/
	void onVisitEvent(DWORD majorErrorCode, DWORD minorErrorCode, wstring url, wstring applicationPath);
	/*
		Function: onProcessEvent

		Method which binds to the <ProcessMonitor> process event slot. This is called
		whenever a malicious process event occurs on the system
	*/
	void onProcessEvent(BOOLEAN created, wstring time, 
						DWORD parentProcessId, wstring parentProcess, 
						DWORD processId, wstring process);
	/*
		Function: onRegistryEvent

		Method which binds to the <RegistryMonitor> registry event slot. Called when
		ever a malcious registry event occurs
	*/
	void onRegistryEvent(wstring registryEventType, wstring time, 
						 wstring processPath, wstring registryEventPath);
	/*
		Function: onFileEvent

		Method which binds to the <FileMonitor> file event slot. Called when
		ever a malcious file event occurs
	*/
	void onFileEvent(wstring fileEventType, wstring time, 
						 wstring processPath, wstring fileEventPath);

	/*
		Function: onOptionChanged

		Called when an option changes in OptionsManager
	*/
	void onOptionChanged(wstring option);
private:
	/*
		Variable: malcious

		Whether or not the system is in a malicious state
	*/
	bool malicious;
	/*
		Variable: collectModifiedFiles

		Whether or not the anaylyzer has been asked to collect all of the
		modified files that occur during the objects start() state.
	*/
	bool collectModifiedFiles;
	bool captureNetworkPackets;
	/*
		Method: compressLogDirectory

		If collectModifiedFiles is true, this method will compress the log directory
		so that it can be saved or sent to the server. It uses both tar.exe and gzip.exe
		to perform this functionality
	*/
	bool compressLogDirectory(wstring logFileName);

	/*
		Method: sendSystemEvent

		Helper method which parses the monitor events into a readible XML document
		which can be saved to a file as a CSV or sent to the server.
	*/
	void sendSystemEvent(wstring* type, wstring* time, wstring* process, wstring* action, wstring* object);
	/*
		Method: sendVisitEvent

		Helper method which parses a visit event from <onVisitEvent> and sends it to the
		server.
	*/
	void sendVisitEvent(wstring* type, wstring* time, 
						 wstring* url, wstring* classification, wstring* application,
						 wstring* majorErrorCode, wstring* minorErrorCode);

	wstring errorCodeToString(DWORD errorCode);

	/*
		Variable: visitor

		Contains the <Visitor> component
	*/	
	Visitor* visitor;
	/*
		Variable: server

		Contains the <Server> component. The analyzer is the only object allowed to
		send data to the remote server
	*/	
	Server* server;
	/*
		Variable: processMonitor

		Pointer to a <ProcessMonitor> instance
	*/	
	ProcessMonitor* processMonitor;
	/*
		Variable: registryMonitor

		Pointer to a <RegistryMonitor> instance
	*/		
	RegistryMonitor* registryMonitor;
	/*
		Variable: fileMonitor

		Pointer to a <FileMonitor> instance
	*/
	FileMonitor* fileMonitor;
	/*
		Variable: networkPacketDumper

		If required this contains a pointer to a <NetworkPacketDumper>. This is only
		loaded when required and only if WinPCAP is installed on the system
	*/	
	NetworkPacketDumper* networkPacketDumper;

	/*
		Variable: on*EventConnection

		Various connections to the slots of monitors. These are an easy way to stop
		listening for various methods
	*/	
	boost::signals::connection onProcessEventConnection;
	boost::signals::connection onRegistryEventConnection;
	boost::signals::connection onFileEventConnection;
	boost::signals::connection onOptionChangedConnection;
};
