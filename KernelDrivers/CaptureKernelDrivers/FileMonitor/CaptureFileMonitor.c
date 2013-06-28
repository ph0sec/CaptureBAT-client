/*
 *	PROJECT: Capture
 *	FILE: CaptureFileMonitor.c
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
//#define WINVER _WIN32_WINNT_WINXP
//#define _WIN32_WINNT 0x0502
//#define _WIN32_WINNT _WIN32_WINNT_WINXP
//#define NTDDI_VERSION NTDDI_WINXPSP2
//#define FLT_MGR_BASELINEd
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
//#include <ntddk.h>

#include <fltkernel.h>
#include <ntstrsafe.h>

typedef unsigned int UINT;

#define FILE_DEVICE_UNKNOWN             0x00000022
#define IOCTL_UNKNOWN_BASE              FILE_DEVICE_UNKNOWN
#define IOCTL_CAPTURE_GET_FILEEVENTS	  CTL_CODE(FILE_DEVICE_UNKNOWN, 0x804, METHOD_NEITHER,FILE_READ_DATA | FILE_WRITE_DATA)
#define CAPTURE_FILEMON_PORT_NAME                   L"\\CaptureFileMonitorPort"
#define USERSPACE_CONNECTION_TIMEOUT 10
#define FILE_POOL_TAG 'cfm'
#define FILE_PACKET_POOL_TAG 'cfmP'
#define FILE_EVENT_POOL_TAG 'cfmE'

/* Naughty ... The following 2 definitions are not documented functions or structures so may
   change at any time */
typedef struct _THREAD_BASIC_INFORMATION { 
	NTSTATUS ExitStatus; 
	PVOID TebBaseAddress; 
	ULONG UniqueProcessId; 
	ULONG UniqueThreadId; 
	KAFFINITY AffinityMask; 
	KPRIORITY BasePriority; 
	ULONG DiffProcessPriority; 
} THREAD_BASIC_INFORMATION, *PTHREAD_BASIC_INFORMATION;

NTSYSAPI NTSTATUS NTAPI ZwQueryInformationThread (IN HANDLE ThreadHandle, 
												  IN THREADINFOCLASS ThreadInformationClass, 
												  OUT PVOID ThreadInformation, 
												  IN ULONG ThreadInformationLength, 
												  OUT PULONG ReturnLength OPTIONAL );

FLT_POSTOP_CALLBACK_STATUS PostFileOperationCallback ( IN OUT PFLT_CALLBACK_DATA Data, 
				IN PCFLT_RELATED_OBJECTS FltObjects, 
				IN PVOID CompletionContext, 
				IN FLT_POST_OPERATION_FLAGS Flags);
				
FLT_PREOP_CALLBACK_STATUS
PreFileOperationCallback (
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __deref_out_opt PVOID *CompletionContext
    );

NTSTATUS FilterUnload ( IN FLT_FILTER_UNLOAD_FLAGS Flags );

NTSTATUS SetupCallback (IN PCFLT_RELATED_OBJECTS  FltObjects,
				IN FLT_INSTANCE_SETUP_FLAGS  Flags,
				IN DEVICE_TYPE  VolumeDeviceType,
				IN FLT_FILESYSTEM_TYPE  VolumeFilesystemType);

NTSTATUS MessageCallback (
    __in PVOID ConnectionCookie,
    __in_bcount_opt(InputBufferSize) PVOID InputBuffer,
    __in ULONG InputBufferSize,
    __out_bcount_part_opt(OutputBufferSize,*ReturnOutputBufferLength) PVOID OutputBuffer,
    __in ULONG OutputBufferSize,
    __out PULONG ReturnOutputBufferLength
    );

NTSTATUS ConnectCallback(
    __in PFLT_PORT ClientPort,
    __in PVOID ServerPortCookie,
    __in_bcount(SizeOfContext) PVOID ConnectionContext,
    __in ULONG SizeOfContext,
    __deref_out_opt PVOID *ConnectionCookie
    );

VOID DisconnectCallback(
    __in_opt PVOID ConnectionCookie
    );
// Callback that this driver will be listening for. This is sent to the filter manager
// where it is registered
CONST FLT_OPERATION_REGISTRATION Callbacks[] = {
	
    { IRP_MJ_CREATE,
      0,
      PreFileOperationCallback,
      PostFileOperationCallback },

    { IRP_MJ_CREATE_NAMED_PIPE,
      0,
      PreFileOperationCallback,
      PostFileOperationCallback },

    { IRP_MJ_CLOSE,
      0,
      PreFileOperationCallback,
      PostFileOperationCallback },

    { IRP_MJ_READ,
      0,
      PreFileOperationCallback,
      PostFileOperationCallback },

    { IRP_MJ_WRITE,
      0,
      PreFileOperationCallback,
      PostFileOperationCallback },

    { IRP_MJ_QUERY_INFORMATION,
      0,
      PreFileOperationCallback,
      PostFileOperationCallback },

    { IRP_MJ_SET_INFORMATION,
      0,
      PreFileOperationCallback,
      PostFileOperationCallback },

    { IRP_MJ_QUERY_EA,
      0,
      PreFileOperationCallback,
      PostFileOperationCallback },

    { IRP_MJ_SET_EA,
      0,
      PreFileOperationCallback,
      PostFileOperationCallback },

    { IRP_MJ_FLUSH_BUFFERS,
      0,
      PreFileOperationCallback,
      PostFileOperationCallback },

    { IRP_MJ_QUERY_VOLUME_INFORMATION,
      0,
      PreFileOperationCallback,
      PostFileOperationCallback },

    { IRP_MJ_SET_VOLUME_INFORMATION,
      0,
      PreFileOperationCallback,
      PostFileOperationCallback },

    { IRP_MJ_DIRECTORY_CONTROL,
      0,
      PreFileOperationCallback,
      PostFileOperationCallback },

    { IRP_MJ_FILE_SYSTEM_CONTROL,
      0,
      PreFileOperationCallback,
      PostFileOperationCallback },

    { IRP_MJ_DEVICE_CONTROL,
      0,
      PreFileOperationCallback,
      PostFileOperationCallback },

    { IRP_MJ_INTERNAL_DEVICE_CONTROL,
      0,
      PreFileOperationCallback,
      PostFileOperationCallback },

    { IRP_MJ_SHUTDOWN,
      0,
      PreFileOperationCallback,
      NULL },

    { IRP_MJ_LOCK_CONTROL,
      0,
      PreFileOperationCallback,
      PostFileOperationCallback },

    { IRP_MJ_CLEANUP,
      0,
      PreFileOperationCallback,
      PostFileOperationCallback },

    { IRP_MJ_CREATE_MAILSLOT,
      0,
      PreFileOperationCallback,
      PostFileOperationCallback },

    { IRP_MJ_QUERY_SECURITY,
      0,
      PreFileOperationCallback,
      PostFileOperationCallback },

    { IRP_MJ_SET_SECURITY,
      0,
      PreFileOperationCallback,
      PostFileOperationCallback },

    { IRP_MJ_QUERY_QUOTA,
      0,
      PreFileOperationCallback,
      PostFileOperationCallback },

    { IRP_MJ_SET_QUOTA,
      0,
      PreFileOperationCallback,
      PostFileOperationCallback },

    { IRP_MJ_PNP,
      0,
      PreFileOperationCallback,
      PostFileOperationCallback },

    { IRP_MJ_ACQUIRE_FOR_SECTION_SYNCHRONIZATION,
      0,
      PreFileOperationCallback,
      PostFileOperationCallback },

    { IRP_MJ_RELEASE_FOR_SECTION_SYNCHRONIZATION,
      0,
      PreFileOperationCallback,
      PostFileOperationCallback },

    { IRP_MJ_ACQUIRE_FOR_MOD_WRITE,
      0,
      PreFileOperationCallback,
      PostFileOperationCallback },

    { IRP_MJ_RELEASE_FOR_MOD_WRITE,
      0,
      PreFileOperationCallback,
      PostFileOperationCallback },

    { IRP_MJ_ACQUIRE_FOR_CC_FLUSH,
      0,
      PreFileOperationCallback,
      PostFileOperationCallback },

    { IRP_MJ_RELEASE_FOR_CC_FLUSH,
      0,
      PreFileOperationCallback,
      PostFileOperationCallback },

    { IRP_MJ_FAST_IO_CHECK_IF_POSSIBLE,
      0,
      PreFileOperationCallback,
      PostFileOperationCallback },

    { IRP_MJ_NETWORK_QUERY_OPEN,
      0,
      PreFileOperationCallback,
      PostFileOperationCallback },

    { IRP_MJ_MDL_READ,
      0,
      PreFileOperationCallback,
      PostFileOperationCallback },

    { IRP_MJ_MDL_READ_COMPLETE,
      0,
      PreFileOperationCallback,
      PostFileOperationCallback },

    { IRP_MJ_PREPARE_MDL_WRITE,
      0,
      PreFileOperationCallback,
      PostFileOperationCallback },

    { IRP_MJ_MDL_WRITE_COMPLETE,
      0,
      PreFileOperationCallback,
      PostFileOperationCallback },

    { IRP_MJ_VOLUME_MOUNT,
      0,
      PreFileOperationCallback,
      PostFileOperationCallback },

    { IRP_MJ_VOLUME_DISMOUNT,
      0,
      PreFileOperationCallback,
      PostFileOperationCallback },

    { IRP_MJ_OPERATION_END }
};

/* File event */
typedef struct  _FILE_EVENT {
	UCHAR majorFileEventType;
	UCHAR minorFileEventType;
	NTSTATUS status;
	ULONG information;
	ULONG flags;
	TIME_FIELDS time;
	HANDLE processId;
	UINT filePathLength;
	WCHAR filePath[];
} FILE_EVENT, *PFILE_EVENT;

/* Storage for file event to be put into a linked list */
typedef struct  _FILE_EVENT_PACKET {
    LIST_ENTRY Link;
	PFILE_EVENT pFileEvent;
} FILE_EVENT_PACKET, * PFILE_EVENT_PACKET; 

typedef enum _FILEMONITOR_COMMAND {
    GetFileEvents,
	SetupMonitor
} FILEMONITOR_COMMAND;

typedef struct _FILEMONITOR_SETUP {
	BOOLEAN bCollectDeletedFiles;
	INT nLogDirectorySize;
	WCHAR wszLogDirectory[1024];
} FILEMONITOR_SETUP, *PFILEMONITOR_SETUP;

typedef struct _FILEMONITOR_MESSAGE {
    FILEMONITOR_COMMAND Command;
} FILEMONITOR_MESSAGE, *PFILEMONITOR_MESSAGE;


const FLT_CONTEXT_REGISTRATION Contexts[] = {
    { FLT_CONTEXT_END }
};

CONST FLT_REGISTRATION FilterRegistration = {
    sizeof(FLT_REGISTRATION),
    FLT_REGISTRATION_VERSION,
    0,
    Contexts,
    Callbacks,
    FilterUnload,
    SetupCallback,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

typedef struct _CAPTURE_FILE_MANAGER
{
	PDRIVER_OBJECT pDriverObject;
	PFLT_FILTER pFilter;
	PFLT_PORT pServerPort;
	PFLT_PORT pClientPort;
	LIST_ENTRY lQueuedFileEvents;
	KSPIN_LOCK  lQueuedFileEventsSpinLock;
	KTIMER connectionCheckerTimer;
	KDPC connectionCheckerFunction;
	BOOLEAN bReady;
	BOOLEAN bCollectDeletedFiles;
	UNICODE_STRING logDirectory;
	ULONG lastContactTime;
} CAPTURE_FILE_MANAGER, *PCAPTURE_FILE_MANAGER;


BOOLEAN QueueFileEvent(PFILE_EVENT pFileEvent);
VOID UpdateLastContactTime();
ULONG GetCurrentTime();
VOID ConnectionChecker(
    IN struct _KDPC  *Dpc,
    IN PVOID  DeferredContext,
    IN PVOID  SystemArgument1,
    IN PVOID  SystemArgument2
    );
VOID DeferredCopyFunction(IN PFLT_DEFERRED_IO_WORKITEM WorkItem, 
					 IN PFLT_CALLBACK_DATA CallbackData, 
					 IN PVOID CompletionContext);
NTSTATUS CopyFile(
		 PFLT_CALLBACK_DATA Data,
		 PCFLT_RELATED_OBJECTS FltObjects,
		 PUNICODE_STRING pCompleteFileName
		 );
VOID
GetDosDeviceName(PDEVICE_OBJECT pDeviceObject,
				 PUNICODE_STRING pShareName,
				 PUNICODE_STRING pDosName);
VOID CreateADirectory(PFLT_INSTANCE pInstance, 
				PUNICODE_STRING pDirectoryPath);
VOID CreateAllDirectories(PFLT_INSTANCE pInstance, PUNICODE_STRING pRootDirectory, PUNICODE_STRING directory);
/* Global variables */
CAPTURE_FILE_MANAGER fileManager;
BOOLEAN busy;

/*	Main entry point into the driver, is called when the driver is loaded */
NTSTATUS DriverEntry(
	IN PDRIVER_OBJECT DriverObject, 
	IN PUNICODE_STRING RegistryPath
	)
{
	NTSTATUS status;
	OBJECT_ATTRIBUTES objectAttributes;
	PSECURITY_DESCRIPTOR pSecurityDescriptor;
	UNICODE_STRING fileMonitorPortName;
	LARGE_INTEGER fileEventsTimeout;
	busy = FALSE;
	fileManager.bReady = FALSE;
	fileManager.bCollectDeletedFiles = FALSE;
	KeInitializeSpinLock(&fileManager.lQueuedFileEventsSpinLock);
	InitializeListHead(&fileManager.lQueuedFileEvents);
	fileManager.pDriverObject = DriverObject;
	//fileManager.logDirectory.Buffer = NULL;

	/* Register driver with the filter manager */
	status = FltRegisterFilter(DriverObject, &FilterRegistration, &fileManager.pFilter);

	if (!NT_SUCCESS(status))
	{
		DbgPrint("CaptureFileMonitor: ERROR FltRegisterFilter - %08x\n", status); 
		return status;
	}

	status  = FltBuildDefaultSecurityDescriptor( &pSecurityDescriptor, FLT_PORT_ALL_ACCESS );

	if (!NT_SUCCESS(status))
	{
		DbgPrint("CaptureFileMonitor: ERROR FltBuildDefaultSecurityDescriptor - %08x\n", status);
		return status;
	}

	RtlInitUnicodeString( &fileMonitorPortName, CAPTURE_FILEMON_PORT_NAME );
	InitializeObjectAttributes( &objectAttributes,
								&fileMonitorPortName,
								OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
								NULL,
								pSecurityDescriptor);

	/* Create the communications port to communicate with the user space process (pipe) */
	status = FltCreateCommunicationPort( fileManager.pFilter,
                                             &fileManager.pServerPort,
                                             &objectAttributes,
                                             NULL,
                                             ConnectCallback,
                                             DisconnectCallback,
                                             MessageCallback,
                                             1 );

	FltFreeSecurityDescriptor(pSecurityDescriptor);

	/* Start the filtering */
	if(NT_SUCCESS(status))
	{
		status = FltStartFiltering(fileManager.pFilter);
		if(!NT_SUCCESS(status))
		{
			DbgPrint("CaptureFileMonitor: ERROR FltStartFiltering - %08x\n", status);
		}
	} else {
		DbgPrint("CaptureFileMonitor: ERROR FltCreateCommunicationPort - %08x\n", status);
	}

	/* If there was a problem during initialisation of the filter then uninstall everything */
	if(!NT_SUCCESS(status))
	{
		if (fileManager.pServerPort != NULL)
			FltCloseCommunicationPort(fileManager.pServerPort);
		if (fileManager.pFilter != NULL)
			FltUnregisterFilter(fileManager.pFilter);
		return status;
	}

	UpdateLastContactTime();

	/* Create a DPC routine so that it can be called periodically */
	KeInitializeDpc(&fileManager.connectionCheckerFunction, 
		(PKDEFERRED_ROUTINE)ConnectionChecker, &fileManager);
	KeInitializeTimer(&fileManager.connectionCheckerTimer);
	fileEventsTimeout.QuadPart = 0;

	/* Set the ConnectionChecker routine to be called every so often */
	KeSetTimerEx(&fileManager.connectionCheckerTimer, 
		fileEventsTimeout, 
		(USERSPACE_CONNECTION_TIMEOUT+(USERSPACE_CONNECTION_TIMEOUT/2))*1000, 
		&fileManager.connectionCheckerFunction);

	fileManager.bReady = TRUE;
	fileManager.logDirectory.Length = 0;
	fileManager.logDirectory.Buffer = NULL;
	DbgPrint("CaptureFileMonitor: Successfully Loaded\n");

	return STATUS_SUCCESS;
}

VOID FreeQueuedEvents()
{
	while(!IsListEmpty(&fileManager.lQueuedFileEvents))
	{
		PLIST_ENTRY head = ExInterlockedRemoveHeadList(&fileManager.lQueuedFileEvents, &fileManager.lQueuedFileEventsSpinLock);
		PFILE_EVENT_PACKET pFileEventPacket = CONTAINING_RECORD(head, FILE_EVENT_PACKET, Link);
		ExFreePoolWithTag(pFileEventPacket->pFileEvent, FILE_POOL_TAG);
		ExFreePoolWithTag(pFileEventPacket, FILE_POOL_TAG);
	}
}

/* Checks to see if the file monitor has received an IOCTL from a userspace
   program in a while. If it hasn't then all old queued file events are
   cleared. This is called periodically when the driver is loaded */
VOID ConnectionChecker(
    IN struct _KDPC  *Dpc,
    IN PVOID  DeferredContext,
    IN PVOID  SystemArgument1,
    IN PVOID  SystemArgument2
    )
{
	if(	(GetCurrentTime()-fileManager.lastContactTime) > (USERSPACE_CONNECTION_TIMEOUT+(USERSPACE_CONNECTION_TIMEOUT/2)))
	{
		DbgPrint("CaptureFileMonitor: WARNING Userspace IOCTL timeout, clearing old queued registry events\n");
		FreeQueuedEvents();
	}
}



NTSTATUS
CopyFile(
		 PFLT_CALLBACK_DATA Data,
		 PCFLT_RELATED_OBJECTS FltObjects,
		 PUNICODE_STRING pCompleteFileName
		 )
{
	NTSTATUS status;
	UNICODE_STRING tempDeletedFilePath;
	OBJECT_ATTRIBUTES tempDeletedObject;
	IO_STATUS_BLOCK ioStatusTempDeleted;
	LARGE_INTEGER allocate;
	FILE_STANDARD_INFORMATION fileStandardInformation;
	HANDLE tempDeletedHandle;
	ULONG returnedLength;
	allocate.QuadPart = 0x10000;

	
	
	InitializeObjectAttributes(
		&tempDeletedObject,
		pCompleteFileName,
		OBJ_CASE_INSENSITIVE,
		NULL,
		NULL
		);
	status = FltQueryInformationFile(
		FltObjects->Instance,
		Data->Iopb->TargetFileObject,
		&fileStandardInformation,
		sizeof(FILE_STANDARD_INFORMATION),
		FileStandardInformation,
		&returnedLength
		);
	if(NT_SUCCESS(status))
	{
		allocate.QuadPart = fileStandardInformation.AllocationSize.QuadPart;
	} else {
		DbgPrint("CaptureFileMonitor: ERROR - Could not get files allocation size\n");
		return status;
	}

	status = FltCreateFile(
		FltObjects->Filter,
		NULL,
		&tempDeletedHandle,
		GENERIC_WRITE,
		&tempDeletedObject,
		&ioStatusTempDeleted,
		&allocate,
		FILE_ATTRIBUTE_NORMAL,
		0,
		FILE_CREATE,
		FILE_NON_DIRECTORY_FILE,
		NULL,
		0,
		0
		);

	if(NT_SUCCESS(status))
	{
		PVOID handleFileObject;
		PVOID pFileBuffer;
		LARGE_INTEGER offset;
	
		ULONG bytesRead = 0;
		ULONG bytesWritten = 0;
		offset.QuadPart = 0;
		status = ObReferenceObjectByHandle(
			tempDeletedHandle,
			0,
			NULL,
			KernelMode,
			&handleFileObject,
			NULL);
		if(!NT_SUCCESS(status))
		{
			DbgPrint("CaptureFileMonitor: ERROR - ObReferenceObjectByHandle - FAILED - %08x\n", status);
			return status;
		}
		
		pFileBuffer = ExAllocatePoolWithTag(NonPagedPool, 65536, FILE_POOL_TAG);
		
		if(pFileBuffer != NULL)
		{
			ObReferenceObject(Data->Iopb->TargetFileObject);
			do {
				IO_STATUS_BLOCK IoStatusBlock;
				bytesWritten = 0;
				status = FltReadFile(
					FltObjects->Instance,
					Data->Iopb->TargetFileObject,
					&offset,
					65536,
					pFileBuffer,
					FLTFL_IO_OPERATION_DO_NOT_UPDATE_BYTE_OFFSET,
					&bytesRead ,
					NULL,
					NULL
					);
			
				if(NT_SUCCESS(status) && bytesRead > 0)
				{
					/* You can't use FltWriteFile here */
					/* Instance may not be the same instance we want to write to eg a
					   flash drive writing a file to a ntfs partition */
					status = ZwWriteFile(
						tempDeletedHandle,
						NULL,
						NULL,
						NULL,
						&IoStatusBlock,
						pFileBuffer,
						bytesRead,
						&offset,
						NULL
						);
					if(NT_SUCCESS(status))
					{
						//DbgPrint("WriteFile: FltReadFile - %08x\n", status);
					}
					/*
					status = FltWriteFile(
						FltObjects->Instance,
						handleFileObject,
						&offset,
						bytesRead,
						pFileBuffer,
						FLTFL_IO_OPERATION_DO_NOT_UPDATE_BYTE_OFFSET,
						&bytesWritten,
						NULL,
						NULL
						);
						*/
				} else {
					//DbgPrint("CopyFile: FltReadFile - %08x\n", status);
					break;
				}
				offset.QuadPart += bytesRead;
			} while(bytesRead == 65536);
			ObDereferenceObject(Data->Iopb->TargetFileObject);
			ExFreePoolWithTag(pFileBuffer, FILE_POOL_TAG);
		}
		ObDereferenceObject(handleFileObject);
		FltClose(tempDeletedHandle);
	} else {
		if(status != STATUS_OBJECT_NAME_COLLISION)
		{
			DbgPrint("CaptureFileMonitor: ERROR - FltCreateFile FAILED - %08x\n",status);
			return status;
		}
	}
	return STATUS_SUCCESS;
}

VOID
CreateADirectory(PFLT_INSTANCE pInstance,
				PUNICODE_STRING pDirectoryPath)
{
	NTSTATUS status;
	OBJECT_ATTRIBUTES directoryAttributesObject;
	IO_STATUS_BLOCK directoryIoStatusBlock;
	LARGE_INTEGER allocate;
	HANDLE hDirectory;
	allocate.QuadPart = 0x1000;
	
	InitializeObjectAttributes(
		&directoryAttributesObject,
		pDirectoryPath,
		OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
		NULL,
		NULL
		);

	/* You can't use FltCreateFile here */
	/* Instance may not be the same instance we want to write to eg a
	   flash drive writing a file to a ntfs partition */
	status = ZwCreateFile(
		&hDirectory,
		0,
		&directoryAttributesObject,
		&directoryIoStatusBlock,
		&allocate,
		0,
		0,
		FILE_CREATE,
		FILE_DIRECTORY_FILE|FILE_SYNCHRONOUS_IO_NONALERT|FILE_WRITE_THROUGH,
		NULL,
		0
		);
	/*
	status = FltCreateFile(
		fileManager.pFilter,
		pInstance,
		&hDirectory,
		0,
		&directoryAttributesObject,
		&directoryIoStatusBlock,
		&allocate,
		0,
		0,
		FILE_CREATE,
		FILE_DIRECTORY_FILE|FILE_SYNCHRONOUS_IO_NONALERT|FILE_WRITE_THROUGH,
		NULL,
		0,
		0
		);
	*/
	if(NT_SUCCESS(status) && NT_SUCCESS(directoryIoStatusBlock.Status))
	{
		FltClose(hDirectory);
	} else {
		if(status != STATUS_OBJECT_NAME_COLLISION)
		{
			DbgPrint("CaptureFileMonitor: CreateADirectory ERROR %08x & %08x\n", status, directoryIoStatusBlock.Status);
		}
	}
}

VOID
GetDosDeviceName(PDEVICE_OBJECT pDeviceObject,
				 PUNICODE_STRING pShareName,
				 PUNICODE_STRING pDosName)
{
	NTSTATUS status;
	UNICODE_STRING tempDosDrive;

	ObReferenceObject(pDeviceObject);

	status = IoVolumeDeviceToDosName(
		(PVOID)pDeviceObject,
		&tempDosDrive);

	pDosName->Length = 0;

	if(NT_SUCCESS(status))
	{
		int i = 0;
		int wchars = tempDosDrive.Length/2;
		int pos = 0;
		
		pDosName->MaximumLength = tempDosDrive.MaximumLength + 2;
		pDosName->Buffer = ExAllocatePoolWithTag(NonPagedPool, pDosName->MaximumLength, FILE_POOL_TAG);
		
		if(pDosName->Buffer != NULL)
		{
			pDosName->Buffer[pos++] = '\\';
			pDosName->Length += sizeof(WCHAR);
			for(i = 0; i < wchars; i++)
			{
				if(tempDosDrive.Buffer[i] != 0x003A)
				{
					pDosName->Buffer[pos++] = tempDosDrive.Buffer[i];
					pDosName->Length += sizeof(WCHAR); // Unicode is 2-bytes
				}
			}
			ExFreePool(tempDosDrive.Buffer);
		}
	} else {
		if(pShareName != NULL)
		{
			pDosName->MaximumLength = pShareName->MaximumLength;
			pDosName->Buffer = ExAllocatePoolWithTag(NonPagedPool, pDosName->MaximumLength, FILE_POOL_TAG);
			if(pDosName->Buffer != NULL)
			{
				RtlUnicodeStringCopy(pDosName, pShareName);
			}
		} else {
			pDosName->MaximumLength = 30; // Dont change this
			pDosName->Buffer = ExAllocatePoolWithTag(NonPagedPool, pDosName->MaximumLength, FILE_POOL_TAG);
			if(pDosName->Buffer != NULL)
			{
				RtlUnicodeStringCatString(pDosName, L"\\UNKNOWN DRIVE");
			}
		}
	}
	ObDereferenceObject(pDeviceObject);
}

VOID
CreateAllDirectories(PFLT_INSTANCE pInstance, 
				PUNICODE_STRING pRootDirectory, 
				PUNICODE_STRING pDirectory)
{
	UNICODE_STRING totalDirectory;
	UNICODE_STRING path;
	USHORT dirSeperator = 0x005C; // Search for \ as a 16-bit unicode char
	USHORT i = 0;
	USHORT length = pDirectory->Length/2;
	USHORT prevDirPosition = 0;

	//RtlUnicodeStringInit(&path, L"\\??\\I:\\logs\\");

	totalDirectory.Length = 0;
	totalDirectory.MaximumLength = pRootDirectory->MaximumLength + pDirectory->MaximumLength + 2;
	totalDirectory.Buffer = ExAllocatePoolWithTag(NonPagedPool, totalDirectory.MaximumLength, FILE_POOL_TAG);
	
	if(totalDirectory.Buffer == NULL)
	{
		return;
	}
	//RtlAppendUnicodeString(
	//RtlUnicodeStringCatString(&totalDirectory, L"\\??\\I:\\logs\\");
	RtlUnicodeStringCat(&totalDirectory,pRootDirectory);

	for(i = 0; i < length; i++)
	{
		USHORT tempChar = pDirectory->Buffer[i];
		if(dirSeperator == tempChar)
		{
			if(i != 0)
			{
				USHORT size = (i) - (prevDirPosition);
				
				PWSTR dir = ExAllocatePoolWithTag(NonPagedPool, sizeof(WCHAR)*(size+1), FILE_POOL_TAG);

				if(dir == NULL)
				{
					break;
				}

				RtlZeroMemory(dir, sizeof(WCHAR)*(size+1));
				RtlCopyMemory(dir,  pDirectory->Buffer+((prevDirPosition)), size*sizeof(WCHAR));
				dir[size] = '\0';
				RtlUnicodeStringCatString(&totalDirectory, dir);
				//DbgPrint("Creating dir: %wZ\n", &totalDirectory);

				CreateADirectory(pInstance, &totalDirectory);
				
				//RtlUnicodeStringCatString(&totalDirectory, L"\\");

				ExFreePoolWithTag(dir, FILE_POOL_TAG);
				
				prevDirPosition = i;
			}		
		}
	}
	ExFreePoolWithTag(totalDirectory.Buffer, FILE_POOL_TAG);
}

VOID
CopyFileIfBeingDeleted(
						PFLT_CALLBACK_DATA Data,
						__in PCFLT_RELATED_OBJECTS FltObjects,
						PFLT_FILE_NAME_INFORMATION pFileNameInformation
						)
{
	NTSTATUS status;
	BOOLEAN isDirectory;
	FltIsDirectory(Data->Iopb->TargetFileObject,FltObjects->Instance,&isDirectory);
	if(isDirectory)
	{
		return;
	}
	if(Data->Iopb->MajorFunction == IRP_MJ_SET_INFORMATION)
	{
		if(Data->Iopb->Parameters.SetFileInformation.FileInformationClass == FileDispositionInformation)
		{
			PFILE_DISPOSITION_INFORMATION pFileInfo = (PFILE_DISPOSITION_INFORMATION)Data->Iopb->Parameters.SetFileInformation.InfoBuffer;
			
			/* If the file is marked for deletion back it up */
			if(pFileInfo->DeleteFile && FltIsOperationSynchronous(Data))
			{
				//pFileEvent->majorFileEventType = 0x99;
				status = FltParseFileNameInformation(pFileNameInformation);
				
				if(NT_SUCCESS(status))
				{
					if(fileManager.bCollectDeletedFiles)
					{
						UNICODE_STRING completeFilePath;
						//UNICODE_STRING logDirectory;
						UNICODE_STRING fileDirectory;
						UNICODE_STRING fileDosDrive;

						//DbgPrint("On Delete: IoStatus - %08x\n", Data->IoStatus.Status);
						//DbgPrint("On Delete: FinalStatus - %08x\n", Data->Iopb->TargetFileObject->FinalStatus);

						
						//RtlUnicodeStringInit(&logDirectory, L"\\??\\I:\\logs\\deleted_files");

						/* Get the Dos drive name this file event was initiated on */
						GetDosDeviceName(Data->Iopb->TargetFileObject->DeviceObject, &pFileNameInformation->Share, &fileDosDrive);
						
						//DbgPrint("DosDrive: %wZ\n", &fileDosDrive);

						/* Allocate enough room to hold to whole path to the copied file */
						/* <CAPTURE LOG DIRECTORY>\<DOS DRIVE>\<FILE DIRECTORY>\<FILE NAME> */
						completeFilePath.Length = 0;
						completeFilePath.MaximumLength = fileManager.logDirectory.MaximumLength + fileDosDrive.MaximumLength + pFileNameInformation->FinalComponent.MaximumLength + 
							pFileNameInformation->ParentDir.MaximumLength + 2;
						completeFilePath.Buffer = ExAllocatePoolWithTag(NonPagedPool, completeFilePath.MaximumLength, FILE_POOL_TAG);

						if(completeFilePath.Buffer != NULL)
						{
							/* Construct complete file path of where the copy of the deleted file is to be put */
							RtlUnicodeStringCat(&completeFilePath, &fileManager.logDirectory);
							RtlUnicodeStringCat(&completeFilePath, &fileDosDrive);
							RtlUnicodeStringCat(&completeFilePath, &pFileNameInformation->ParentDir);
							RtlUnicodeStringCat(&completeFilePath, &pFileNameInformation->FinalComponent);
						}

						//DbgPrint("ParentDir: %wZ\n", &pFileNameInformation->ParentDir);
						//DbgPrint("FinalComponent: %wZ\n", &pFileNameInformation->FinalComponent);
						//DbgPrint("CompleteFilePath: %wZ\n", &completeFilePath);

						/* Allocate space for the file directory */
						fileDirectory.Length = 0;
						fileDirectory.MaximumLength = pFileNameInformation->ParentDir.MaximumLength + fileDosDrive.MaximumLength + 2;
						fileDirectory.Buffer = ExAllocatePoolWithTag(NonPagedPool, fileDirectory.MaximumLength, FILE_POOL_TAG);					
						
						if(fileDirectory.Buffer != NULL)
						{
							/* Append the dos drive name, and then the directory */
							RtlUnicodeStringCat(&fileDirectory, &fileDosDrive);
							RtlUnicodeStringCat(&fileDirectory, &pFileNameInformation->ParentDir);
						
							//DbgPrint("FileDirectory: %wZ\n", &fileDirectory);
						}

						/* Create all the directories in the fileDirectory string */
						CreateAllDirectories(FltObjects->Instance, &fileManager.logDirectory, &fileDirectory);
						
						/* Path should exist now so copy the file */
						CopyFile(Data, FltObjects, &completeFilePath);
						
						/* Free all the UNICODE_STRING we allocated space for */
						if(completeFilePath.Buffer != NULL)
							ExFreePoolWithTag(completeFilePath.Buffer, FILE_POOL_TAG);
						if(fileDirectory.Buffer != NULL)
							ExFreePoolWithTag(fileDirectory.Buffer, FILE_POOL_TAG);
						if(fileDosDrive.Buffer != NULL)
							ExFreePoolWithTag(fileDosDrive.Buffer, FILE_POOL_TAG);
					}
				} else {
					DbgPrint("CaptureFileMonitor: ERROR - FltParseFileNameInformation - %08x\n", status);
				}
			}	
		}
	}
}

FLT_PREOP_CALLBACK_STATUS
PreFileOperationCallback (
    __inout PFLT_CALLBACK_DATA Data,
    __in PCFLT_RELATED_OBJECTS FltObjects,
    __deref_out_opt PVOID *CompletionContext
    )
{
	NTSTATUS status;
	PFLT_FILE_NAME_INFORMATION pFileNameInformation;
	PFILE_EVENT pFileEvent;
	LARGE_INTEGER CurrentSystemTime;
	LARGE_INTEGER CurrentLocalTime;
	ULONG returnedLength;
	//HANDLE hThread;
	//HANDLE handle5;
	TIME_FIELDS TimeFields;
	BOOLEAN pathNameFound = FALSE;
	UNICODE_STRING filePath;
	
	FLT_PREOP_CALLBACK_STATUS returnStatus = FLT_PREOP_SUCCESS_NO_CALLBACK;
	
	/* If this is a callback for a FS Filter driver then we ignore the event */
	if(FLT_IS_FS_FILTER_OPERATION(Data))
	{
		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}

	/* Allocate a large 64kb string ... maximum path name allowed in windows */
	filePath.Length = 0;
	filePath.MaximumLength = NTSTRSAFE_UNICODE_STRING_MAX_CCH * sizeof(WCHAR);
	filePath.Buffer = ExAllocatePoolWithTag(NonPagedPool, filePath.MaximumLength, FILE_POOL_TAG); 

	if(filePath.Buffer == NULL)
	{
		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}

	if (FltObjects->FileObject != NULL && Data != NULL) {
		status = FltGetFileNameInformation( Data,
											FLT_FILE_NAME_NORMALIZED |
											FLT_FILE_NAME_QUERY_ALWAYS_ALLOW_CACHE_LOOKUP,
											&pFileNameInformation );
		if(NT_SUCCESS(status))
		{
			if(pFileNameInformation->Name.Length > 0)
			{
				RtlUnicodeStringCopy(&filePath, &pFileNameInformation->Name);
				//RtlStringCbCopyUnicodeString(pFileEvent->filePath, 1024, &pFileNameInformation->Name);
				pathNameFound = TRUE;
			}
			/* Backup the file if it is marked for deletion */
			CopyFileIfBeingDeleted (Data,FltObjects,pFileNameInformation);

			/* Release the file name information structure if it was used */
			if(pFileNameInformation != NULL)
			{
				FltReleaseFileNameInformation(pFileNameInformation);
			}
		} else {
			NTSTATUS lstatus;
            PFLT_FILE_NAME_INFORMATION pLFileNameInformation;
            lstatus = FltGetFileNameInformation( Data,
				FLT_FILE_NAME_OPENED | FLT_FILE_NAME_QUERY_ALWAYS_ALLOW_CACHE_LOOKUP,
				&pLFileNameInformation);
			if(NT_SUCCESS(lstatus))
			{
				if(pLFileNameInformation->Name.Length > 0)
				{
					RtlUnicodeStringCopy(&filePath, &pLFileNameInformation->Name);
					//RtlStringCbCopyUnicodeString(pFileEvent->filePath, 1024, &pLFileNameInformation->Name);
					pathNameFound = TRUE;
				}
				/* Backup the file if it is marked for deletion */
				CopyFileIfBeingDeleted (Data,FltObjects,pFileNameInformation);

				/* Release the file name information structure if it was used */
				if(pLFileNameInformation != NULL)
				{
					FltReleaseFileNameInformation(pLFileNameInformation);
				}
			}
		}
	}

	/* If path name could not be found the file monitor uses the file name stored
	   in the FileObject. The documentation says that we shouldn't use this info
	   as it may not be correct. It does however allow us get some nice file events
	   such as the system process writing to the $MFT file etc. Remove this code if
	   problems start to occur */
	if( (pathNameFound == FALSE) && 
		(FltObjects->FileObject != NULL) &&
		(FltObjects->FileObject->RelatedFileObject == NULL) &&
		(FltObjects->FileObject->FileName.Length > 0))
	{
		NTSTATUS status;
		ULONG size;
		UNICODE_STRING szTempPath;
		UNICODE_STRING szDevice;
		UNICODE_STRING szFileNameDevice;

		/* Check the FileObject->FileName isn't already a complete filepath */
		szFileNameDevice.Length = FltObjects->FileObject->FileName.Length;
		szFileNameDevice.MaximumLength = FltObjects->FileObject->FileName.MaximumLength;
		szFileNameDevice.Buffer = ExAllocatePoolWithTag(NonPagedPool, szFileNameDevice.MaximumLength, FILE_POOL_TAG);
		RtlInitUnicodeString(&szDevice, L"\\Device");
		if(FltObjects->FileObject->FileName.Length >= szDevice.Length)
		{
			RtlUnicodeStringCchCopyN(&szFileNameDevice, &FltObjects->FileObject->FileName, 7);
		}

		if(RtlEqualUnicodeString(&szDevice, &szFileNameDevice, TRUE))
		{
			RtlUnicodeStringCopy(&filePath, &FltObjects->FileObject->FileName);
			pathNameFound = TRUE;
		} else {
			szTempPath.Length = 0;
			szTempPath.MaximumLength = FltObjects->FileObject->FileName.MaximumLength + 2;

			/* Get the volume name of where the event came from */
			status = FltGetVolumeName(
						FltObjects->Volume,
						NULL,
						&size
						);
			if(status == STATUS_BUFFER_TOO_SMALL)
			{
				szTempPath.MaximumLength += (USHORT)size;
				szTempPath.Buffer = ExAllocatePoolWithTag(NonPagedPool, szTempPath.MaximumLength, FILE_POOL_TAG);
				
				if(szTempPath.Buffer != NULL)
				{
					status = FltGetVolumeName(
							FltObjects->Volume,
							&szTempPath,
							&size
							);
					if(NT_SUCCESS(status))
					{
						/* Append the file event to the volume name */
						RtlUnicodeStringCat(&szTempPath, &FltObjects->FileObject->FileName);
						RtlUnicodeStringCopy(&filePath, &szTempPath);
						pathNameFound = TRUE;
					}
					ExFreePoolWithTag(szTempPath.Buffer, FILE_POOL_TAG);
				}
			}
		}
		ExFreePoolWithTag(szFileNameDevice.Buffer, FILE_POOL_TAG);
	}

	if(!pathNameFound)
	{
		RtlUnicodeStringCatString(&filePath, L"UNKNOWN"); 
	}

	/* Allocate file event and put the values into it */
	/* NOTE this is freed in the post op callback (which should always get called) */
	pFileEvent = ExAllocatePoolWithTag(NonPagedPool, sizeof(FILE_EVENT)+filePath.Length+sizeof(WCHAR), FILE_POOL_TAG);
	
	if(pFileEvent == NULL)
	{
		ExFreePoolWithTag(filePath.Buffer, FILE_POOL_TAG);
		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}

	/* Copy file path into file event */
	pFileEvent->filePathLength = filePath.Length+sizeof(WCHAR);
	RtlStringCbCopyUnicodeString(pFileEvent->filePath, pFileEvent->filePathLength, &filePath);
	/* Free the allocated storage for a filepath */
	ExFreePoolWithTag(filePath.Buffer, FILE_POOL_TAG);

	pFileEvent->majorFileEventType = Data->Iopb->MajorFunction;
	pFileEvent->minorFileEventType = Data->Iopb->MinorFunction;
	pFileEvent->processId = 0;

	if (FltObjects->FileObject != NULL)
	{
		pFileEvent->flags = FltObjects->FileObject->Flags;
	}

	if(Data->Iopb->MajorFunction == IRP_MJ_SET_INFORMATION)
	{
		if(Data->Iopb->Parameters.SetFileInformation.FileInformationClass == FileDispositionInformation)
		{
			PFILE_DISPOSITION_INFORMATION pFileInfo = (PFILE_DISPOSITION_INFORMATION)Data->Iopb->Parameters.SetFileInformation.InfoBuffer;
			/* If the file is marked for deletion back it up */
			if(pFileInfo->DeleteFile)
			{
				pFileEvent->majorFileEventType = 0x99;
			}
		}
	}

	/* Get the process id of the file event */
	/* NOTE we are kinda using an undocumented function here but its all available
	   on the interweb. Plus its much better than accessing the PETHREAD structure
	   which could change at any time. Also, this one is available to userspace
	   programs so it should be safe to use. We have to use this function because
	   a file I/O may either be processed in the context of the userspace program
	   or the system context. This uses the thread data from FLT_CALLBACK_DATA to
	   determine which process it actually came from. We default back to getting
	   the current process id if all else fails. */
	/* SECOND NOTE FltGetRequestorProcessId does not get the correct process id, it
	   looks like it still get the proces id of the context the pre callback gets
	   called in */
	/*
	status = ObOpenObjectByPointer(Data->Thread, 
		OBJ_KERNEL_HANDLE, 
		NULL, 
		0, 
		0, 
		KernelMode, 
		&hThread);
	if(NT_SUCCESS(status))
	{
		THREAD_BASIC_INFORMATION threadBasicInformation;

		status = ZwQueryInformationThread(hThread, 
			ThreadBasicInformation, 
			&threadBasicInformation, 
			sizeof(THREAD_BASIC_INFORMATION), 
			&returnedLength );
		if(NT_SUCCESS(status))
		{
			pFileEvent->processId = (HANDLE)threadBasicInformation.UniqueProcessId;
			handle5 = pFileEvent->processId;
			//DbgPrint("Process4: %i\n", pFileEvent->processId);
		} else {		
			DbgPrint("ZwQueryInformationThread FAILED: %08x\n", status);
		}
		ZwClose(hThread);
	} else {		
		DbgPrint("ObOpenObjectByPointer FAILED: %08x\n", status);
	}
	*/

	/* New safe get correct process id. One above causes blue screen in some cases */
	if(Data->Thread != NULL)
	{
		PEPROCESS pProcess = IoThreadToProcess( Data->Thread );
		pFileEvent->processId = PsGetProcessId(pProcess);
	} else {
		pFileEvent->processId = PsGetCurrentProcessId();
		DbgPrint("CaptureFileMonitor: Process id may be incorrect\n");
	}
/*
	DbgPrint("%i [%i %i] %s %i %i (%i, %i, %i) %i %i %i %i %i : %ls\n", 
			KeGetCurrentIrql(),
			PsIsSystemThread(Data->Thread),
			PsIsSystemThread(PsGetCurrentThread()),
			FltGetIrpName(Data->Iopb->MajorFunction),
			Data->Iopb->MajorFunction,
			Data->Iopb->MinorFunction,
			pFileEvent->processId,
			PsGetCurrentProcessId(),
			FltGetRequestorProcessId(Data),
			FLT_IS_FASTIO_OPERATION(Data),
			FLT_IS_FS_FILTER_OPERATION(Data),
			FLT_IS_IRP_OPERATION(Data),
			FLT_IS_REISSUED_IO(Data),
			FLT_IS_SYSTEM_BUFFER(Data),
			pFileEvent->filePath);
*/			
	//ASSERT(pFileEvent->processId != 0);

	/* Get the time this event occured */
	KeQuerySystemTime(&CurrentSystemTime);
	ExSystemTimeToLocalTime(&CurrentSystemTime,&CurrentLocalTime);
	RtlTimeToTimeFields(&CurrentLocalTime,&TimeFields);
	pFileEvent->time = TimeFields;



	/* Pass the created file event to the post operation of this pre file operation */
	if (Data->Iopb->MajorFunction == IRP_MJ_SHUTDOWN)
	{
		PostFileOperationCallback( Data,
						FltObjects,
						pFileEvent,
						0 );
		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	} else {
		*CompletionContext = pFileEvent;
		return FLT_PREOP_SUCCESS_WITH_CALLBACK;
	}

    
}

VOID DeferredCopyFunction(IN PFLT_DEFERRED_IO_WORKITEM WorkItem, 
						  IN PFLT_CALLBACK_DATA Data, 
						  IN PVOID CompletionContext)
{

	FltCompletePendedPreOperation(Data,FLT_PREOP_SUCCESS_NO_CALLBACK,CompletionContext); 
	FltFreeDeferredIoWorkItem(WorkItem);
	busy = FALSE;
}

FLT_POSTOP_CALLBACK_STATUS PostFileOperationCallback ( IN OUT PFLT_CALLBACK_DATA Data, 
	IN PCFLT_RELATED_OBJECTS FltObjects, 
	IN PVOID CompletionContext, 
	IN FLT_POST_OPERATION_FLAGS Flags)
{
	/* You cannot properly get the full path name of a file operation in the postop
	   this is documented in the WDK documentation regarding this function. READ IT ...
	   in partcular the Comments section regarding the IRP level that this can run in */
	PFILE_EVENT pFileEvent;

	pFileEvent = (PFILE_EVENT)CompletionContext;

	if(pFileEvent == NULL)
	{
		return FLT_POSTOP_FINISHED_PROCESSING;
	}

	/* If the post file operation is draining then don't worry about adding the event */
	if(FlagOn(Flags, FLTFL_POST_OPERATION_DRAINING))
	{
		if(pFileEvent != NULL)
		{
			ExFreePoolWithTag(pFileEvent, FILE_POOL_TAG);
			pFileEvent = NULL;
		}
		return FLT_POSTOP_FINISHED_PROCESSING;
	}

	pFileEvent->status = Data->IoStatus.Status;
	pFileEvent->information = Data->IoStatus.Information;

	if(!QueueFileEvent(pFileEvent))
	{
		if(pFileEvent != NULL)
		{
			ExFreePoolWithTag(pFileEvent, FILE_POOL_TAG);
			pFileEvent = NULL;
		}
	}
    return FLT_POSTOP_FINISHED_PROCESSING;
}

VOID UpdateLastContactTime()
{
	fileManager.lastContactTime = GetCurrentTime();
}

ULONG GetCurrentTime()
{
	LARGE_INTEGER currentSystemTime;
	LARGE_INTEGER currentLocalTime;
	ULONG time;

	KeQuerySystemTime(&currentSystemTime);
	ExSystemTimeToLocalTime(&currentSystemTime,&currentLocalTime);
	RtlTimeToSecondsSince1970(&currentLocalTime, &time);
	return time;
}

BOOLEAN QueueFileEvent(PFILE_EVENT pFileEvent)
{
	BOOLEAN inserted = FALSE;
	if(	(GetCurrentTime()-fileManager.lastContactTime) <= USERSPACE_CONNECTION_TIMEOUT)
	{
		PFILE_EVENT_PACKET pFileEventPacket;
		
		pFileEventPacket = ExAllocatePoolWithTag(NonPagedPool, sizeof(FILE_EVENT_PACKET), FILE_POOL_TAG);
		if(pFileEventPacket != NULL)
		{
			//RtlCopyMemory(&pFileEventPacket->fileEvent, pFileEvent, sizeof(FILE_EVENT));
			pFileEventPacket->pFileEvent = pFileEvent;
			ExInterlockedInsertTailList(&fileManager.lQueuedFileEvents, &pFileEventPacket->Link, &fileManager.lQueuedFileEventsSpinLock);
			inserted = TRUE;
		}
	}
	return inserted;
}

NTSTATUS FilterUnload ( IN FLT_FILTER_UNLOAD_FLAGS Flags )
{
	PLIST_ENTRY pListHead;
	PFILE_EVENT_PACKET pFileEventPacket;
	FltCloseCommunicationPort( fileManager.pServerPort );

	if(fileManager.bReady == TRUE)
	{
		FltUnregisterFilter( fileManager.pFilter );	
		KeCancelTimer(&fileManager.connectionCheckerTimer);
	}

	// Free the log directory
	if(fileManager.logDirectory.Buffer != NULL)
	{
		ExFreePoolWithTag(fileManager.logDirectory.Buffer, FILE_POOL_TAG);
		fileManager.logDirectory.Buffer = NULL;
	}
	fileManager.bCollectDeletedFiles = FALSE;

	// Free the linked list containing all the left over file events
	FreeQueuedEvents();

	return STATUS_SUCCESS;
}

NTSTATUS SetupCallback (IN PCFLT_RELATED_OBJECTS  FltObjects,
	IN FLT_INSTANCE_SETUP_FLAGS  Flags,
	IN DEVICE_TYPE  VolumeDeviceType,
	IN FLT_FILESYSTEM_TYPE  VolumeFilesystemType)
{
	// Always return true so that we attach to all known volumes and all future volumes (USB flashdrives etc)
	if (VolumeDeviceType == FILE_DEVICE_NETWORK_FILE_SYSTEM) {
       return STATUS_FLT_DO_NOT_ATTACH;
    }

    return STATUS_SUCCESS;
}

NTSTATUS MessageCallback (
    __in PVOID ConnectionCookie,
    __in_bcount_opt(InputBufferSize) PVOID InputBuffer,
    __in ULONG InputBufferSize,
    __out_bcount_part_opt(OutputBufferSize,*ReturnOutputBufferLength) PVOID OutputBuffer,
    __in ULONG OutputBufferSize,
    __out PULONG ReturnOutputBufferLength
    )
{	
	NTSTATUS status;
	FILEMONITOR_COMMAND command;
	PLIST_ENTRY pListHead;
	PFILE_EVENT_PACKET pFileEventPacket;
	UINT bufferSpace = OutputBufferSize;
	UINT bufferSpaceUsed = 0;
	PCHAR pOutputBuffer = OutputBuffer;

	if ((InputBuffer != NULL) &&
		(InputBufferSize >= (FIELD_OFFSET(FILEMONITOR_MESSAGE,Command) +
                             sizeof(FILEMONITOR_COMMAND))))
	{
        try  {
            command = ((PFILEMONITOR_MESSAGE) InputBuffer)->Command;
        } except( EXCEPTION_EXECUTE_HANDLER ) {
            return GetExceptionCode();
        }
		
		switch(command) {
			case GetFileEvents:
				UpdateLastContactTime();
				while(!IsListEmpty(&fileManager.lQueuedFileEvents) && 
					(bufferSpaceUsed < bufferSpace) &&
					((bufferSpace-bufferSpaceUsed) >= sizeof(FILE_EVENT)))
				{
					UINT fileEventSize = 0;

					pListHead = ExInterlockedRemoveHeadList(&fileManager.lQueuedFileEvents, &fileManager.lQueuedFileEventsSpinLock);
					pFileEventPacket = CONTAINING_RECORD(pListHead, FILE_EVENT_PACKET, Link);
					
					// Get the file event size, taking into account the variable path name size
					fileEventSize = sizeof(FILE_EVENT)+pFileEventPacket->pFileEvent->filePathLength;
					if((bufferSpace-bufferSpaceUsed) >= fileEventSize)
					{
						// Copy the memory into the user space buffer
						RtlCopyMemory(pOutputBuffer+bufferSpaceUsed, pFileEventPacket->pFileEvent, fileEventSize);
						bufferSpaceUsed += fileEventSize;
						
						// Free the allocated packet and file event
						ExFreePoolWithTag(pFileEventPacket->pFileEvent, FILE_POOL_TAG);
						ExFreePoolWithTag(pFileEventPacket, FILE_POOL_TAG);
					} else {
						// Not enough user space buffer left so put the packet we just got from the list back
						ExInterlockedInsertHeadList(&fileManager.lQueuedFileEvents, &pFileEventPacket->Link, &fileManager.lQueuedFileEventsSpinLock);
						break;
					}
				}
				
				*ReturnOutputBufferLength = bufferSpaceUsed;
				status = STATUS_SUCCESS;
				break;
			case SetupMonitor:
				if(bufferSpace == sizeof(FILEMONITOR_SETUP))
				{
					PFILEMONITOR_SETUP pFileMonitorSetup = (PFILEMONITOR_SETUP)pOutputBuffer;			
					if(pFileMonitorSetup->bCollectDeletedFiles)
					{
						USHORT logDirectorySize;
						UNICODE_STRING uszTempString;
						
						// Create a temp path name that contains the fudge stuff to open files in kernel space
						RtlInitUnicodeString(&uszTempString, L"\\??\\");
						logDirectorySize = uszTempString.MaximumLength + pFileMonitorSetup->nLogDirectorySize + 2;
						
						// If the log director is already defined free it
						if(fileManager.logDirectory.Buffer != NULL)
						{
							ExFreePoolWithTag(fileManager.logDirectory.Buffer, FILE_POOL_TAG);
							fileManager.logDirectory.Buffer = NULL;
						}

						fileManager.logDirectory.Buffer = ExAllocatePoolWithTag(NonPagedPool, logDirectorySize+sizeof(UNICODE_STRING)+2, FILE_POOL_TAG);
						
						if(fileManager.logDirectory.Buffer != NULL)
						{
							fileManager.logDirectory.Length = 0;
							fileManager.logDirectory.MaximumLength = logDirectorySize;										
							fileManager.bCollectDeletedFiles = pFileMonitorSetup->bCollectDeletedFiles;

							// Copy delete log directory into the file manager
							RtlUnicodeStringCat(&fileManager.logDirectory, &uszTempString);
							RtlUnicodeStringCatString(&fileManager.logDirectory, pFileMonitorSetup->wszLogDirectory);
						
							DbgPrint("CaptureFileMonitor: Collecting deleted files: %wZ\n", &fileManager.logDirectory);
						}
					} else {
						fileManager.bCollectDeletedFiles = FALSE;
						// If we were previously collecting deleted files we must have had a log directory
						// set ... so free it if we did
						if(fileManager.logDirectory.Buffer != NULL)
						{
							ExFreePoolWithTag(fileManager.logDirectory.Buffer, FILE_POOL_TAG);
							fileManager.logDirectory.Buffer = NULL;
						}		
					}
				}
				status = STATUS_SUCCESS;
			default:
				status = STATUS_INVALID_PARAMETER;
				break;

		}
	} else {
		status = STATUS_INVALID_PARAMETER;
	}
	return status;
}

NTSTATUS ConnectCallback(
    __in PFLT_PORT ClientPort,
    __in PVOID ServerPortCookie,
    __in_bcount(SizeOfContext) PVOID ConnectionContext,
    __in ULONG SizeOfContext,
    __deref_out_opt PVOID *ConnectionCookie
    )
{
	ASSERT( fileManager.pClientPort == NULL );
    fileManager.pClientPort = ClientPort;
    return STATUS_SUCCESS;
}

VOID DisconnectCallback(__in_opt PVOID ConnectionCookie)
{
	PLIST_ENTRY pListHead;
	PFILE_EVENT_PACKET pFileEventPacket;
	
	// Free the linked list containing all the left over file events
	FreeQueuedEvents();

	// Free the log directory if one was allocated
	if(fileManager.logDirectory.Buffer != NULL)
	{
		ExFreePoolWithTag(fileManager.logDirectory.Buffer, FILE_POOL_TAG);
		fileManager.logDirectory.Buffer = NULL;
	}
	fileManager.bCollectDeletedFiles = FALSE;

	FltCloseClientPort( fileManager.pFilter, &fileManager.pClientPort );
}
