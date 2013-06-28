/*
 *	PROJECT: Capture
 *	FILE: FileMonitor.h
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
#include <winioctl.h>
#include <boost/signal.hpp>
#include <boost/bind.hpp>
#include <hash_set>
#include <shlobj.h>
#include "Thread.h"
#include "Monitor.h"
#include "MiniFilter.h"

typedef pair <wstring, wstring> DosPair;

#define CAPTURE_FILEMON_PORT_NAME	L"\\CaptureFileMonitorPort"

/*
	Enum: FILEMONITOR_COMMAND

	Commands that are sent through <commPort> to the kernel driver

	GetFileEvents - Retrieve a buffer that contains a list of <FILE_EVENT>'s
	SetupMonitor - Send the monitor setup to the kernel driver. Log directory etc
*/
typedef enum _FILEMONITOR_COMMAND {

    GetFileEvents,
	SetupMonitor

} FILEMONITOR_COMMAND;

/*
	Enum: FILE_NOTIFY_CLASS

	DEPRECIATED - Not used anymore
	
	Enum containing the types of events that the kernel driver is monitoring

	FilePreRead - File read event type
	FilePreWrite - File write event type
*/
typedef enum _FILE_NOTIFY_CLASS {
    FilePreRead,
    FilePreWrite,
	FilePreClose,
	FilePreDelete,
	FilePreCreate
} FILE_NOTIFY_CLASS;

/*
	Struct: FILEMONITOR_MESSAGE

	Contains a command to be sent through to the kernel driver

	Command - Actual command to be sent
*/
typedef struct _FILEMONITOR_MESSAGE {
    FILEMONITOR_COMMAND Command;
} FILEMONITOR_MESSAGE, *PFILEMONITOR_MESSAGE;

typedef struct _FILEMONITOR_SETUP {
	BOOLEAN bCollectDeletedFiles;
	UINT nLogDirectorySize;
	WCHAR wszLogDirectory[1024];
} FILEMONITOR_SETUP, *PFILEMONITOR_SETUP;

/*
	Struct: FILE_EVENT

	File event that contains what event happened on what file and by what process

	type - file event type of <FILE_NOTIFY_CLASS>
	time - time when the event occured in the kernel
	name - Contains the name of the event (file modified)
	processID - The process id that caused the file event
*/
typedef struct  _FILE_EVENT {
	UCHAR majorFileEventType;
	UCHAR minorFileEventType;
	ULONG status;
	ULONG information;
	ULONG flags;
	TIME_FIELDS time;
	DWORD processId;
	UINT filePathLength;
	WCHAR filePath[];
} FILE_EVENT, *PFILE_EVENT;
#define FlagOn(_F,_SF)        ((_F) & (_SF))
/* File event status */
#define FILE_SUPERSEDED                 0x00000000
#define FILE_OPENED                     0x00000001
#define FILE_CREATED                    0x00000002
#define FILE_OVERWRITTEN                0x00000003
#define FILE_EXISTS                     0x00000004
#define FILE_DOES_NOT_EXIST             0x00000005

/* File event flags */
#define FO_FILE_OPEN                    0x00000001
#define FO_SYNCHRONOUS_IO               0x00000002
#define FO_ALERTABLE_IO                 0x00000004
#define FO_NO_INTERMEDIATE_BUFFERING    0x00000008
#define FO_WRITE_THROUGH                0x00000010
#define FO_SEQUENTIAL_ONLY              0x00000020
#define FO_CACHE_SUPPORTED              0x00000040
#define FO_NAMED_PIPE                   0x00000080
#define FO_STREAM_FILE                  0x00000100
#define FO_MAILSLOT                     0x00000200
#define FO_GENERATE_AUDIT_ON_CLOSE      0x00000400
#define FO_QUEUE_IRP_TO_THREAD          FO_GENERATE_AUDIT_ON_CLOSE
#define FO_DIRECT_DEVICE_OPEN           0x00000800
#define FO_FILE_MODIFIED                0x00001000
#define FO_FILE_SIZE_CHANGED            0x00002000
#define FO_CLEANUP_COMPLETE             0x00004000
#define FO_TEMPORARY_FILE               0x00008000
#define FO_DELETE_ON_CLOSE              0x00010000
#define FO_OPENED_CASE_SENSITIVE        0x00020000
#define FO_HANDLE_CREATED               0x00040000
#define FO_FILE_FAST_IO_READ            0x00080000
#define FO_RANDOM_ACCESS                0x00100000
#define FO_FILE_OPEN_CANCELLED          0x00200000
#define FO_VOLUME_OPEN                  0x00400000
#define FO_REMOTE_ORIGIN                0x01000000
#define FO_SKIP_COMPLETION_PORT         0x02000000
#define FO_SKIP_SET_EVENT               0x04000000
#define FO_SKIP_SET_FAST_IO             0x08000000

/* Max Buffer size to allocate */
#define FILE_EVENTS_BUFFER_SIZE 5*65536
/* Normal wait time to request events from the kernel driver */
#define FILE_EVENT_WAIT_TIME 50
/* If the buffer was semi full then we wait a lesser time to get the new events */
#define FILE_EVENT_BUFFER_FULL_WAIT_TIME 5

/*
	Class: FileMonitor

	The file monitor is responsible for interacting with the CaptureFileMonitor
	minifilter driver (CaptureFileMonitor.c). This is responsible for communicating
	with the driver which passes a user-space allocated buffer into kernel-space 
	which the driver will then fill with file events. It also keeps track of which
	files have been modified so that when required it can copy those files into
	a temporary directory. When a event is received the monitor checks to see if
	it is excluded. If it is not excluded it is malicious and the onFileEvent slot
	is signalled with the event information.

	The FileMonitor listens for "file-exclusion" events from the <EventController>
	so that the server or any external object can add/remove exclusions from the
	monitors lists.

	Implements: Runnable - So that events can be received in the background
				Monitor - Has access to exclusion lists and loading kernel drivers
*/
class FileMonitor : public Runnable, public Monitor
{
	public:
	typedef boost::signal<void (wstring, wstring, wstring, wstring)> signal_fileEvent;
public:
	FileMonitor(void);
	virtual ~FileMonitor(void);

	void start();
	void stop();
	void run();

	inline bool isMonitorRunning() { return monitorRunning; }
	inline bool isDriverInstalled() { return driverInstalled; }

	void copyCreatedFiles();
	void setMonitorModifiedFiles(bool monitor);

	void onFileExclusionReceived(Element* pElement);

	boost::signals::connection connect_onFileEvent(const signal_fileEvent::slot_type& s);

private:
	bool getFileEventName(PFILE_EVENT pFileEvent, wstring* fileEventName);
	wstring convertFileObjectNameToDosName(wstring fileObjectName);
	void initialiseDosNameMap();
	bool isDirectory(wstring filePath);
	void createFilePathAndCopy(wstring* logPath, wstring* filePath);

	BYTE* fileEvents;
	Thread* fileMonitorThread;
	HANDLE hDriver;
	HANDLE communicationPort;
	HANDLE hMonitorStoppedEvent;
	signal_fileEvent signal_onFileEvent;
	stdext::hash_map<wstring, wstring> dosNameMap;
	stdext::hash_set<wstring> modifiedFiles;
	bool monitorRunning;
	bool driverInstalled;
	bool monitorModifiedFiles;

	boost::signals::connection onFileExclusionReceivedConnection;
};
