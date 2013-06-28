#include "Logger.h"

Logger::Logger(void)
{
	fileOpen = false;
	hLog = INVALID_HANDLE_VALUE;
}

Logger::~Logger(void)
{
	instanceCreated = false;
}

Logger*
Logger::getInstance()
{
	if(!instanceCreated)
	{
		logger = new Logger();
		instanceCreated = true;	
	}
	return logger;
}

char* 
Logger::convertToMultiByteString(wstring* message, size_t* charsConverted)
{
	*charsConverted = 0;
	int length = static_cast<int>(message->length());
	int mbsize = WideCharToMultiByte(CP_UTF8, 0, message->c_str(), length, NULL, 0, NULL, NULL);
	if (mbsize > 0)
	{
		char* szMessage = (char*)malloc(mbsize);
		int bytes = WideCharToMultiByte(CP_UTF8, 0, message->c_str(), length, szMessage, mbsize, NULL, NULL);
		if(bytes == 0)
		{
			DebugPrint(L"Logger-convertToMultiByteString: WideCharToMultiByte ERROR - %08x allocated: %i\n", GetLastError(), mbsize);
		}
		*charsConverted = bytes;
		return szMessage;
	} else {
		DebugPrint(L"Logger-convertToMultiByteString: WideCharToMultiByte ERROR - %08x allocated: %i\n", GetLastError(), mbsize);
	}
	return NULL;
}

void
Logger::writeSystemEventToLog(wstring* type, wstring* time, 
							  wstring* process, wstring* action, 
							  wstring* object)
{
	if(isFileOpen())
	{
		wstring message = L"\"";
		message += *time;
		message += L"\",\"";
		message += *type;
		message += L"\",\"";
		message += *action;
		message += L"\",\"";
		message += *process;
		message += L"\",\"";
		message += *object;
		message += L"\"\r\n";
		writeToLog(&message);
	}
}

void 
Logger::writeToLog(wstring* message)
{
	if(isFileOpen())
	{
		size_t charsConverted;
		char* szMessage = convertToMultiByteString(message, &charsConverted);
		if(szMessage != NULL)
		{
			DWORD bytesWritten;
			WriteFile(
				hLog,
				szMessage,
				static_cast<unsigned long>(charsConverted), // Ignore null charater
				&bytesWritten,
				NULL
				);

			free(szMessage);
		}
	}
}

void
Logger::openLogFile(wstring file)
{
	if(!isFileOpen())
	{
		wchar_t* szFullPath = new wchar_t[4096];
		wchar_t* pFileName;
		GetFullPathName(
			file.c_str(),
			4096,
			szFullPath,
			&pFileName
		);
		logFullPath = szFullPath;
		logFileName = pFileName;
		std::transform(logFullPath.begin(),logFullPath.end(),logFullPath.begin(), towlower);
		hLog = CreateFile(
					szFullPath,
					GENERIC_WRITE,
					FILE_SHARE_READ,
					NULL,
					CREATE_ALWAYS,
					FILE_ATTRIBUTE_NORMAL,
					NULL
					);
		if(hLog != INVALID_HANDLE_VALUE)
		{
			DebugPrint(L"Logger-openLogFile: Log file successfully opened - %ls\n", szFullPath);
			fileOpen = true;
		} else {
			printf("Logger: ERROR %08x - Could not open log file %ls\n", GetLastError(), szFullPath);
		}
		delete [] szFullPath;
	}
}

void
Logger::closeLogFile()
{
	if(isFileOpen())
	{
		CloseHandle(hLog);
		fileOpen = false;
	}
}