/*
 *	PROJECT: Capture
 *	FILE: RegistryMonitor.h
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
#include "Thread.h"
#include "Monitor.h"

typedef enum _REG_NOTIFY_CLASS {
    RegNtDeleteKey,
    RegNtPreDeleteKey = RegNtDeleteKey,
    RegNtSetValueKey,
    RegNtPreSetValueKey = RegNtSetValueKey,
    RegNtDeleteValueKey,
    RegNtPreDeleteValueKey = RegNtDeleteValueKey,
    RegNtSetInformationKey,
    RegNtPreSetInformationKey = RegNtSetInformationKey,
    RegNtRenameKey,
    RegNtPreRenameKey = RegNtRenameKey,
    RegNtEnumerateKey,
    RegNtPreEnumerateKey = RegNtEnumerateKey,
    RegNtEnumerateValueKey,
    RegNtPreEnumerateValueKey = RegNtEnumerateValueKey,
    RegNtQueryKey,
    RegNtPreQueryKey = RegNtQueryKey,
    RegNtQueryValueKey,
    RegNtPreQueryValueKey = RegNtQueryValueKey,
    RegNtQueryMultipleValueKey,
    RegNtPreQueryMultipleValueKey = RegNtQueryMultipleValueKey,
    RegNtPreCreateKey,
    RegNtPostCreateKey,
    RegNtPreOpenKey,
    RegNtPostOpenKey,
    RegNtKeyHandleClose,
    RegNtPreKeyHandleClose = RegNtKeyHandleClose,
    //
    // The following values apply only to Microsoft Windows Server 2003 and later.
    //    
    RegNtPostDeleteKey,
    RegNtPostSetValueKey,
    RegNtPostDeleteValueKey,
    RegNtPostSetInformationKey,
    RegNtPostRenameKey,
    RegNtPostEnumerateKey,
    RegNtPostEnumerateValueKey,
    RegNtPostQueryKey,
    RegNtPostQueryValueKey,
    RegNtPostQueryMultipleValueKey,
    RegNtPostKeyHandleClose,
    RegNtPreCreateKeyEx,
    RegNtPostCreateKeyEx,
    RegNtPreOpenKeyEx,
    RegNtPostOpenKeyEx,
    //
    // The following values apply only to Microsoft Windows Vista and later.
    //    
    RegNtPreFlushKey,
    RegNtPostFlushKey,
    RegNtPreLoadKey,
    RegNtPostLoadKey,
    RegNtPreUnLoadKey,
    RegNtPostUnLoadKey,
    RegNtPreQueryKeySecurity,
    RegNtPostQueryKeySecurity,
    RegNtPreSetKeySecurity,
    RegNtPostSetKeySecurity,
    RegNtCallbackContextCleanup,
    MaxRegNtNotifyClass 
} REG_NOTIFY_CLASS;

/*
typedef struct _REGISTRY_EVENT
{
	int type;
	TIME_FIELDS time;
	WCHAR name[1024];
	UINT processID;
} REGISTRY_EVENT, *PREGISTRY_EVENT;
*/

typedef struct  _REGISTRY_EVENT {
	REG_NOTIFY_CLASS eventType;
	TIME_FIELDS time;	
	DWORD processId;
	ULONG dataType;
	ULONG dataLengthB;
	ULONG registryPathLengthB;
	/* Contains path and optionally data */
	UCHAR registryData[];
} REGISTRY_EVENT, * PREGISTRY_EVENT;

#define REGISTRY_EVENTS_BUFFER_SIZE 5*65536
#define REGISTRY_DEFAULT_WAIT_TIME 65
#define REGISTRY_BUFFER_FULL_WAIT_TIME 5

struct UNICODE_STRING
{
	WORD Length;
	WORD MaximumLength;
	PWSTR Buffer;
};

typedef struct __PUBLIC_OBJECT_TYPE_INFORMATION {
    UNICODE_STRING TypeName;
    ULONG Reserved [22];    // reserved for internal use
} PUBLIC_OBJECT_TYPE_INFORMATION, *PPUBLIC_OBJECT_TYPE_INFORMATION;

#define IOCTL_GET_REGISTRY_EVENTS      CTL_CODE(FILE_DEVICE_UNKNOWN, 0x803, METHOD_NEITHER,FILE_READ_DATA | FILE_WRITE_DATA)
#define NTSTATUS ULONG
#define STATUS_INFO_LENGTH_MISMATCH ((NTSTATUS)0xC0000004L)


typedef pair <wstring, wstring> ObjectPair;

/*
	Class: RegistryMonitor
   
	Manages the CaptureRegistryMonitor kernel driver. It runs in a thread which continuasly
	requests registry events from the kernel driver. When events are received the events
	are checked for exclusion and passed onto all objects which are attached to the
	onRegistryEvent slot. When the buffer if full of events the monitor will wait a less
	time before requesting more events. This is to reduce the amount of queued registry
	events in kernel space. As much as 10,000 events in a second can occur, and if they
	are all queued up they are going to use up a lot of nonpaged kernel memory ... which
	is bad.

	The registry kernel driver has been somewhat redesigned to stop this from happening. If
	it doesn't receive a userspace buffer in a certain amount of time the queued events
	are deleted and the kernel memory is freed.
*/
class RegistryMonitor : public Runnable, public Monitor
{
public:
	typedef boost::signal<void (wstring, wstring, wstring, wstring)> signal_registryEvent;
public:
	RegistryMonitor(void);
	virtual ~RegistryMonitor(void);

	void start();
	void stop();
	void run();

	bool isMonitorRunning() { return monitorRunning; }
	bool isDriverInstalled() { return driverInstalled; }

	void onRegistryExclusionReceived(Element* pElement);

	boost::signals::connection connect_onRegistryEvent(const signal_registryEvent::slot_type& s);
private:
	wstring getRegistryEventName(int registryEventType);
	wstring convertRegistryObjectNameToHiveName(wstring registryObjectName);
	void initialiseObjectNameMap();

	boost::signals::connection onRegistryExclusionReceivedConnection;

	HANDLE hEvent;
	HANDLE hDriver;
	HANDLE hMonitorStoppedEvent;
	BYTE* registryEventsBuffer;
	Thread* registryMonitorThread;
	signal_registryEvent signal_onRegistryEvent;
	std::list<ObjectPair> objectNameMap;
	bool driverInstalled;
	bool monitorRunning;
};
