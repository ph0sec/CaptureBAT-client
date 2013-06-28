#include "FileDownloader.h"

FileDownloader::FileDownloader(void)
{
	hDownloaded= CreateEvent(NULL, FALSE, FALSE, NULL);
	hConnected = CreateEvent(NULL, FALSE, FALSE, NULL);
	bDownloading = false;
}

FileDownloader::~FileDownloader(void)
{
	CloseHandle(hDownloaded);
	CloseHandle(hConnected);
}

DWORD
FileDownloader::Download(wstring what, wstring *toFile)
{
	DWORD dwStatus;
	BOOL folderCreated = FALSE;

	// Setup temp file paths etc 
	wstring tempFilePath = L"temp\\";
	//tempFilePath = *toFile;
	wchar_t* szTempPath = new wchar_t[1024];
	wchar_t* szTempFilePath = new wchar_t[1024];
	GetFullPathName(tempFilePath.c_str(), 1024, szTempFilePath, NULL);
	GetFullPathName(L"temp", 1024, szTempPath, NULL);
	tempFilePath = szTempFilePath;
	folderCreated = CreateDirectory(szTempPath, NULL);
	delete [] szTempPath;
	delete [] szTempFilePath;

	if(folderCreated == FALSE)
	{
		//printf("FileDownloader - ERROR - Could not create temp directory: %ls\n", tempPath.c_str());
		//return false;
	}
	tempFilePath += L"tempFile";
	
	wstring extension = what.substr(what.find_last_of(L"."));
	tempFilePath += extension;

	DeleteFile(tempFilePath.c_str());

	printf("Downloading to: %ls\n", tempFilePath.c_str());
	*toFile = tempFilePath;
	HRESULT result = URLDownloadToFile(NULL,
			what.c_str(),
			tempFilePath.c_str(),
			0,
			this
		);

	// Wait for connection signal
	dwStatus = WaitForSingleObject(hConnected, 30000);

	if(dwStatus == WAIT_TIMEOUT)
	{
		printf("FileDownloader - ERROR - Connection timeout\n");
		return CAPTURE_NE_CONNECT_ERROR_DL_TEMP_FILE;
	}
	printf("FileDownloader - Downloading file: %ls\n", what.c_str());

	// Wait for download to finish signal
	dwStatus = WaitForSingleObject(hDownloaded, 60000);
	if(dwStatus == WAIT_TIMEOUT)
	{
		printf("FileDownloader - ERROR - Download timeout\n");
		return CAPTURE_NE_CANT_DOWNLOAD_TEMP_FILE;
	}
	printf("FileDownloader - File Downloaded: %ls\n", what.c_str());
	return 0;
}
