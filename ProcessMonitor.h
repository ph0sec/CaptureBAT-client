/*
 *	PROJECT: Capture
 *	FILE: ProcessMonitor.h
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
#include <boost/signal.hpp>
#include <boost/bind.hpp>
#include "Monitor.h"
#include "Thread.h"


#define IOCTL_CAPTURE_GET_PROCINFO	CTL_CODE(0x00000022, 0x0802, METHOD_NEITHER, FILE_READ_DATA | FILE_WRITE_DATA)
#define IOCTL_CAPTURE_PROC_LIST    CTL_CODE(0x00000022, 0x0807, METHOD_NEITHER, FILE_READ_DATA | FILE_WRITE_DATA)

typedef struct _ProcessInfo
{
	TIME_FIELDS time;
    DWORD  ParentId;
    DWORD  ProcessId;
    BOOLEAN bCreate;
	UINT processPathLength;
	WCHAR processPath[1024];
} ProcessInfo;

typedef struct _PROCESS_TUPLE
{
	DWORD processID;
	WCHAR name[1024];
} PROCESS_TUPLE, * PPROCESS_TUPLE;

/*
	Class: ProcessMonitor

	Manages the CaptureProcessMonitor kernel driver. It waits for a kernel event to
	be singalled which tells it that a process event has occured. This is retrieved
	from kernel space and parsed into a process event which is passed onto the
	<ProcessManager> and then checked for exclusion and passed onto all objects which
	are attached to the onProcessEvent slot

	Implements: <IRunnable>, <VisitorListener>, <Monitor>
*/
class ProcessMonitor : public Runnable, public Monitor
{
public:
	typedef boost::signal<void (BOOLEAN, wstring, DWORD, wstring, DWORD, wstring)> signal_processEvent;
public:
	ProcessMonitor(void);
	virtual ~ProcessMonitor(void);

	void start();
	void stop();
	void run();

	bool isMonitorRunning() { return monitorRunning; }
	bool isDriverInstalled() { return driverInstalled; }

	void onProcessExclusionReceived(Element* pElement);

	boost::signals::connection connect_onProcessEvent(const signal_processEvent::slot_type& s);
private:

	void initialiseKernelDriverProcessMap();
	
	HANDLE hEvent;
	HANDLE hDriver;
	HANDLE hMonitorStoppedEvent;
	Thread* processMonitorThread;
	signal_processEvent signalProcessEvent;
	bool driverInstalled;
	bool monitorRunning;
	boost::signals::connection processManagerConnection;
	boost::signals::connection onProcessExclusionReceivedConnection;
};
