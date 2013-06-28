/*
 *	PROJECT: Capture
 *	FILE: CaptureProcessMonitor.c
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

/*	
	Capture Process Monitor - Kernel Driver
	Based on code from James M. Finnegan - http://www.microsoft.com/msj/0799/nerd/nerd0799.aspx

	Driver for monitoring process on Windows XP (should work on all NT systems)

	By Ramon Steenson (rsteenson@gmail.com)
*/
#define NTDDI_WINXPSP2                      0x05010200
#define OSVERSION_MASK      0xFFFF0000
#define SPVERSION_MASK      0x0000FF00
#define SUBVERSION_MASK     0x000000FF


//
// macros to extract various version fields from the NTDDI version
//
#define OSVER(Version)  ((Version) & OSVERSION_MASK)
#define SPVER(Version)  (((Version) & SPVERSION_MASK) >> 8)
#define SUBVER(Version) (((Version) & SUBVERSION_MASK) )
//#define NTDDI_VERSION   NTDDI_WINXPSP2
//#include <sdkddkver.h>
#include <ntifs.h>
#include <wdm.h>
#include <ntstrsafe.h>

#define FILE_DEVICE_UNKNOWN             0x00000022
#define IOCTL_UNKNOWN_BASE              FILE_DEVICE_UNKNOWN
#define IOCTL_CAPTURE_GET_PROCINFO    CTL_CODE(IOCTL_UNKNOWN_BASE, 0x0802, METHOD_NEITHER, FILE_READ_DATA | FILE_WRITE_DATA)

#define IOCTL_CAPTURE_PROC_LIST    CTL_CODE(IOCTL_UNKNOWN_BASE, 0x0807, METHOD_NEITHER, FILE_READ_DATA | FILE_WRITE_DATA)
#define PROCESS_POOL_TAG 'pPR'
#define USERSPACE_CONNECTION_TIMEOUT 10
#define PROCESS_HASH_SIZE 1024
#define PROCESS_HASH(ProcessId) \
	((UINT)(((UINT)ProcessId) % PROCESS_HASH_SIZE))

typedef unsigned int UINT;
typedef char * PCHAR;
typedef PVOID POBJECT;

typedef struct  _IMAGE_INFORMATION {
	ULONG kernelModeImage;
	WCHAR imagePath[1024];
} IMAGE_INFORMATION, * PIMAGE_INFORMATION;

/* Image packet */
typedef struct  _IMAGE_PACKET {
    LIST_ENTRY     Link;
	IMAGE_INFORMATION imageInformation;
} IMAGE_PACKET, * PIMAGE_PACKET; 

/* Process event */
typedef struct  _PROCESS_INFORMATION {
	HANDLE processId;
	WCHAR processPath[1024];
	LIST_ENTRY lLoadedImageList;
} PROCESS_INFORMATION, * PPROCESS_INFORMATION;


/* Storage for process information to be put into the process hash map */
typedef struct  _PROCESS_PACKET {
    LIST_ENTRY     Link;
	PROCESS_INFORMATION processInformation;
} PROCESS_PACKET, * PPROCESS_PACKET; 

typedef struct _PROCESS_HASH_ENTRY
{
	LIST_ENTRY lProcess;
	KSPIN_LOCK  lProcessSpinLock;
} PROCESS_HASH_ENTRY, * PPROCESS_HASH_ENTRY;

/* Structure to be passed to the kernel driver using openevent */
typedef struct _PROCESS_EVENT
{
	TIME_FIELDS time;
    HANDLE  hParentProcessId;
    HANDLE  hProcessId;
    BOOLEAN bCreated;
	UINT processPathLength;
	WCHAR processPath[1024];
} PROCESS_EVENT, *PPROCESS_EVENT;

typedef struct  _PROCESS_EVENT_PACKET {
    LIST_ENTRY     Link;
	PROCESS_EVENT processEvent;

} PROCESS_EVENT_PACKET, * PPROCESS_EVENT_PACKET; 

typedef NTSTATUS (*QUERY_INFO_PROCESS) (
    __in HANDLE ProcessHandle,
    __in PROCESSINFOCLASS ProcessInformationClass,
    __out_bcount(ProcessInformationLength) PVOID ProcessInformation,
    __in ULONG ProcessInformationLength,
    __out_opt PULONG ReturnLength
    );

QUERY_INFO_PROCESS ZwQueryInformationProcess;

/* Context stuff */
typedef struct _CAPTURE_PROCESS_MANAGER 
{
    PDEVICE_OBJECT pDeviceObject;
	BOOLEAN bReady;
	PKEVENT eNewProcessEvent;
	HANDLE  hNewProcessEvent;
	PPROCESS_EVENT pCurrentProcessEvent;
	FAST_MUTEX mProcessWaitingSpinLock;
	ULONG lastContactTime;
} CAPTURE_PROCESS_MANAGER, * PCAPTURE_PROCESS_MANAGER;

/* Methods */
NTSTATUS KDispatchCreateClose(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS KDispatchIoctl(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

void UnloadDriver(PDRIVER_OBJECT DriverObject);

VOID ProcessCallback(IN HANDLE  hParentId, IN HANDLE  hProcessId, IN BOOLEAN bCreate);
VOID ProcessImageCallback (
    IN PUNICODE_STRING  FullImageName,
    IN HANDLE  ProcessId, // where image is mapped
    IN PIMAGE_INFO  ImageInfo
    );

BOOLEAN InsertProcess(PROCESS_INFORMATION processInformation);
BOOLEAN RemoveProcess(HANDLE processId);
PLIST_ENTRY FindProcess(HANDLE processId);
PPROCESS_INFORMATION GetProcess(HANDLE processId);

VOID UpdateLastContactTime();
ULONG GetCurrentTime();
VOID QueueCurrentProcessEvent(PPROCESS_EVENT pProcessEvent);

/* Global process manager so our process callback can use the information*/
PDEVICE_OBJECT gpDeviceObject;

/*	Main entry point into the driver, is called when the driver is loaded */
NTSTATUS DriverEntry(
	IN PDRIVER_OBJECT DriverObject, 
	IN PUNICODE_STRING RegistryPath
	)
{
    NTSTATUS        ntStatus;
    UNICODE_STRING  uszDriverString;
    UNICODE_STRING  uszDeviceString;
    UNICODE_STRING  uszProcessEventString;
    PDEVICE_OBJECT    pDeviceObject;
	PCAPTURE_PROCESS_MANAGER pProcessManager;
	int i;
    
	/* Point uszDriverString at the driver name */
    RtlInitUnicodeString(&uszDriverString, L"\\Device\\CaptureProcessMonitor");

    /* Create and initialise Process Monitor device object */
    ntStatus = IoCreateDevice(
		DriverObject,
        sizeof(CAPTURE_PROCESS_MANAGER),
        &uszDriverString,
        FILE_DEVICE_UNKNOWN,
        0,
        FALSE,
        &pDeviceObject
		);
    if(!NT_SUCCESS(ntStatus)) {
		DbgPrint("CaptureProcessMonitor: ERROR IoCreateDevice ->  \\Device\\CaptureProcessMonitor - %08x\n", ntStatus); 
        return ntStatus;
	}

	/* Point uszDeviceString at the device name */
    RtlInitUnicodeString(&uszDeviceString, L"\\DosDevices\\CaptureProcessMonitor");
    
	/* Create symbolic link to the user-visible name */
    ntStatus = IoCreateSymbolicLink(&uszDeviceString, &uszDriverString);

    if(!NT_SUCCESS(ntStatus))
    {
		DbgPrint("CaptureProcessMonitor: ERROR IoCreateSymbolicLink ->  \\DosDevices\\CaptureProcessMonitor - %08x\n", ntStatus); 
        IoDeleteDevice(pDeviceObject);
        return ntStatus;
    }

	/* Set global device object to newly created object */
	gpDeviceObject = pDeviceObject;

	/* Get the process manager from the extension of the device */
	pProcessManager = gpDeviceObject->DeviceExtension;

    /* Assign global pointer to the device object for use by the callback functions */
    pProcessManager->pDeviceObject = pDeviceObject;
	ExInitializeFastMutex(&pProcessManager->mProcessWaitingSpinLock);

	/* Create event for user-mode processes to monitor */
    RtlInitUnicodeString(&uszProcessEventString, L"\\BaseNamedObjects\\CaptureProcDrvProcessEvent");
    pProcessManager->eNewProcessEvent = IoCreateNotificationEvent (&uszProcessEventString, &pProcessManager->hNewProcessEvent);
	KeClearEvent(pProcessManager->eNewProcessEvent);

    /* Load structure to point to IRP handlers */
    DriverObject->DriverUnload                         = UnloadDriver;
    DriverObject->MajorFunction[IRP_MJ_CREATE]         = KDispatchCreateClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]          = KDispatchCreateClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = KDispatchIoctl;

	pProcessManager->pCurrentProcessEvent = NULL;
    /* Register process callback function */
	ntStatus = PsSetCreateProcessNotifyRoutine(ProcessCallback, FALSE);
	if(!NT_SUCCESS(ntStatus))
	{
		DbgPrint("CaptureProcessMonitor: ERROR PsSetCreateProcessNotifyRoutine - %08x\n", ntStatus); 
		return ntStatus;
	}

	/* Process Manager is ready to receive processes */
	pProcessManager->bReady = TRUE;
    
	DbgPrint("CaptureProcessMonitor: Successfully Loaded\n"); 
	
	/* Return success */
    return STATUS_SUCCESS;
}

NTSTATUS KDispatchCreateClose(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information=0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

NTSTATUS GetProcessImageName(HANDLE processId, PUNICODE_STRING ProcessImageName)
{
    NTSTATUS status;
    ULONG returnedLength;
    ULONG bufferLength;
	HANDLE hProcess;
    PVOID buffer;
	PEPROCESS eProcess;
    PUNICODE_STRING imageName;
   
    PAGED_CODE(); // this eliminates the possibility of the IDLE Thread/Process
	
	status = PsLookupProcessByProcessId(processId, &eProcess);

	if(NT_SUCCESS(status))
	{
		status = ObOpenObjectByPointer(eProcess,0, NULL, 0,0,KernelMode,&hProcess);
		if(NT_SUCCESS(status))
		{
		} else {
			DbgPrint("ObOpenObjectByPointer Failed: %08x\n", status);
		}
		ObDereferenceObject(eProcess);
	} else {
		DbgPrint("PsLookupProcessByProcessId Failed: %08x\n", status);
	}
	

    if (NULL == ZwQueryInformationProcess) {

        UNICODE_STRING routineName;

        RtlInitUnicodeString(&routineName, L"ZwQueryInformationProcess");

        ZwQueryInformationProcess =
               (QUERY_INFO_PROCESS) MmGetSystemRoutineAddress(&routineName);

        if (NULL == ZwQueryInformationProcess) {
            DbgPrint("Cannot resolve ZwQueryInformationProcess\n");
        }
    }
    
	/* Query the actual size of the process path */
    status = ZwQueryInformationProcess( hProcess,
                                        ProcessImageFileName,
                                        NULL, // buffer
                                        0, // buffer size
                                        &returnedLength);

    if (STATUS_INFO_LENGTH_MISMATCH != status) {
        return status;
    }

    /* Check there is enough space to store the actual process
	   path when it is found. If not return an error with the
	   required size */
    bufferLength = returnedLength - sizeof(UNICODE_STRING);
    if (ProcessImageName->MaximumLength < bufferLength)
	{
        ProcessImageName->MaximumLength = (USHORT) bufferLength;
        return STATUS_BUFFER_OVERFLOW;   
    }

    /* Allocate a temporary buffer to store the path name */
    buffer = ExAllocatePoolWithTag(NonPagedPool, returnedLength, PROCESS_POOL_TAG);

    if (NULL == buffer) 
	{
        return STATUS_INSUFFICIENT_RESOURCES;   
    }

    /* Retrieve the process path from the handle to the process */
    status = ZwQueryInformationProcess( hProcess,
                                        ProcessImageFileName,
                                        buffer,
                                        returnedLength,
                                        &returnedLength);

    if (NT_SUCCESS(status)) 
	{
        /* Copy the path name */
        imageName = (PUNICODE_STRING) buffer;
        RtlCopyUnicodeString(ProcessImageName, imageName);
    }

    /* Free the temp buffer which stored the path */
    ExFreePoolWithTag(buffer, PROCESS_POOL_TAG);

    return status;
}

/*	
	Process Callback that is called every time a process event occurs. Creates
	a kernel event which can be used to notify userspace processes. 
*/
VOID ProcessCallback(
	IN HANDLE  hParentId, 
	IN HANDLE  hProcessId, 
	IN BOOLEAN bCreate
	)
{
	NTSTATUS status;
	LARGE_INTEGER currentSystemTime;
	LARGE_INTEGER currentLocalTime;
	TIME_FIELDS timeFields;
	UNICODE_STRING processImagePath;
	PCAPTURE_PROCESS_MANAGER pProcessManager;

	/* Get the current time */
	KeQuerySystemTime(&currentSystemTime);
	ExSystemTimeToLocalTime(&currentSystemTime,&currentLocalTime);
	RtlTimeToTimeFields(&currentLocalTime,&timeFields);

    /* Get the process manager from the device extension */
    pProcessManager = gpDeviceObject->DeviceExtension;

	pProcessManager->pCurrentProcessEvent = ExAllocatePoolWithTag(NonPagedPool, sizeof(PROCESS_EVENT), PROCESS_POOL_TAG);

	if(pProcessManager->pCurrentProcessEvent == NULL)
	{
		return;
	}

	RtlCopyMemory(&pProcessManager->pCurrentProcessEvent->time, &timeFields, sizeof(TIME_FIELDS));

	processImagePath.Length = 0;
	processImagePath.MaximumLength = 0;

	status = GetProcessImageName(hProcessId, &processImagePath);
	if(status == STATUS_BUFFER_OVERFLOW)
	{
		processImagePath.Buffer = ExAllocatePoolWithTag(NonPagedPool, processImagePath.MaximumLength, PROCESS_POOL_TAG);
		if(processImagePath.Buffer != NULL)
		{
			status = GetProcessImageName(hProcessId, &processImagePath);
			if(NT_SUCCESS(status))
			{
				DbgPrint("CaptureProcessMonitor: %i %i=>%i:%wZ\n", bCreate, hParentId, hProcessId, &processImagePath);
				RtlStringCbCopyUnicodeString(pProcessManager->pCurrentProcessEvent->processPath, 1024, &processImagePath);
			}
			ExFreePoolWithTag(processImagePath.Buffer,PROCESS_POOL_TAG);
		}
	}

	pProcessManager->pCurrentProcessEvent->hParentProcessId = hParentId;
	pProcessManager->pCurrentProcessEvent->hProcessId = hProcessId;
	pProcessManager->pCurrentProcessEvent->bCreated = bCreate;

	KeSetEvent(pProcessManager->eNewProcessEvent, 0, FALSE);
	KeClearEvent(pProcessManager->eNewProcessEvent);
}


NTSTATUS KDispatchIoctl(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    NTSTATUS              status = STATUS_UNSUCCESSFUL;
    PIO_STACK_LOCATION    irpStack  = IoGetCurrentIrpStackLocation(Irp);
    PPROCESS_EVENT        pProcessEvent;
	PCHAR pOutputBuffer;
	PPROCESS_INFORMATION pInputBuffer;
	UINT dwDataWritten = 0;
	PCAPTURE_PROCESS_MANAGER pProcessManager;

	/* Get the process manager from the device extension */
    pProcessManager = gpDeviceObject->DeviceExtension;

    switch(irpStack->Parameters.DeviceIoControl.IoControlCode)
    {
        case IOCTL_CAPTURE_GET_PROCINFO:
			/* Update the time the user space program last sent an IOCTL */
			//UpdateLastContactTime();
			/* Return some of the process events that are queued */
            if(irpStack->Parameters.DeviceIoControl.OutputBufferLength >= sizeof(PROCESS_EVENT))
            {
				ULONG left = irpStack->Parameters.DeviceIoControl.OutputBufferLength;
				ULONG done = 0;
				pOutputBuffer = Irp->UserBuffer;
				__try {
					ProbeForWrite(pOutputBuffer, 
								irpStack->Parameters.DeviceIoControl.OutputBufferLength,
								__alignof (PROCESS_EVENT));
					ExAcquireFastMutex(&pProcessManager->mProcessWaitingSpinLock);
					if(pProcessManager->pCurrentProcessEvent != NULL)
					{
						RtlCopyMemory(pOutputBuffer+done, pProcessManager->pCurrentProcessEvent, sizeof(PROCESS_EVENT));
						done += sizeof(PROCESS_EVENT);
						ExFreePoolWithTag(pProcessManager->pCurrentProcessEvent, PROCESS_POOL_TAG);
						pProcessManager->pCurrentProcessEvent = NULL;
					}
					ExReleaseFastMutex(&pProcessManager->mProcessWaitingSpinLock);
					/*
					while(!IsListEmpty(&pProcessManager->lQueuedProcessEvents) && (done < left))
					{
						PLIST_ENTRY head;
						PPROCESS_EVENT_PACKET pProcessEventPacket;
						head = ExInterlockedRemoveHeadList(&pProcessManager->lQueuedProcessEvents, &pProcessManager->lQueuedProcessEventsSpinLock);
						pProcessEventPacket = CONTAINING_RECORD(head, PROCESS_EVENT_PACKET, Link);
						
						RtlCopyMemory(pOutputBuffer+done, &pProcessEventPacket->processEvent, sizeof(PROCESS_EVENT));
						done += sizeof(PROCESS_EVENT);

						ExFreePool(pProcessEventPacket);
					}
					*/
					dwDataWritten = done;
					status = STATUS_SUCCESS;
				} __except( EXCEPTION_EXECUTE_HANDLER ) {
					DbgPrint("CaptureProcessMonitor: EXCEPTION IOCTL_CAPTURE_GET_PROCINFO - %08x\n", GetExceptionCode());
					status = GetExceptionCode();     
				}        
            }
            break;
        default:
            break;
    }
    Irp->IoStatus.Status = status;
   
    // Set # of bytes to copy back to user-mode...
    if(status == STATUS_SUCCESS)
        Irp->IoStatus.Information = dwDataWritten;
    else
        Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}

void UnloadDriver(IN PDRIVER_OBJECT DriverObject)
{
    UNICODE_STRING uszDeviceString;
	NTSTATUS ntStatus;
	PCAPTURE_PROCESS_MANAGER pProcessManager;
	int i;

	/* Get the process manager from the device extension */
    pProcessManager = gpDeviceObject->DeviceExtension;

    /* Remove the callback routines */
	if(pProcessManager->bReady)
	{
		ntStatus = PsSetCreateProcessNotifyRoutine(ProcessCallback, TRUE);
		//PsRemoveLoadImageNotifyRoutine(ProcessImageCallback);
		pProcessManager->bReady = FALSE;
	}

	ExAcquireFastMutex(&pProcessManager->mProcessWaitingSpinLock);
	if(pProcessManager->pCurrentProcessEvent != NULL)
	{
		ExFreePoolWithTag(pProcessManager->pCurrentProcessEvent, PROCESS_POOL_TAG);
		pProcessManager->pCurrentProcessEvent = NULL;
	}
	ExReleaseFastMutex(&pProcessManager->mProcessWaitingSpinLock);

	RtlInitUnicodeString(&uszDeviceString, L"\\DosDevices\\CaptureProcessMonitor");
	/* Delete the symbolic link */
    IoDeleteSymbolicLink(&uszDeviceString);

	/* Delete the device */
	if(DriverObject->DeviceObject != NULL)
	{
		IoDeleteDevice(DriverObject->DeviceObject);
	}
}