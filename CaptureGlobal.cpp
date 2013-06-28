#include "CaptureGlobal.h"
#include <strsafe.h>

wstring
CaptureGlobal::urlEncode(wstring text)
{
	size_t len = text.length();
	wstring encoded = L"";
	for(size_t i = 0; i < len; i++)
	{
		wchar_t wch = text.at(i);
		if ('A' <= wch && wch <= 'Z') {
			encoded += wch;
		} else if ('a' <= wch && wch <= 'z') {
			encoded += wch;
		} else if ('0' <= wch && wch <= '9') {
			encoded += wch;
		} else if (wch == ' ') {
			encoded += L"+";
		} else if (wch == '-' || wch == '_'
			|| wch == '.' || wch == '!'
			|| wch == '~' || wch == '*'
			|| wch == '\'' || wch == '('
			|| wch == ')') {
			encoded += wch;
		} else if (wch <= 0x007f) {		// other ASCII
			encoded += hexenc[wch];
		} else if (wch <= 0x07FF) {		// non-ASCII <= 0x7FF
			encoded += hexenc[0xc0 | (wch >> 6)];
			encoded += hexenc[0x80 | (wch & 0x3F)];
		} else {					// 0x7FF < ch <= 0xFFFF
			encoded += hexenc[0xe0 | (wch >> 12)];
			encoded += hexenc[0x80 | ((wch >> 6) & 0x3F)];
			encoded += hexenc[0x80 | (wch & 0x3F)];
		}
	}
	return encoded;
}

wstring
CaptureGlobal::urlDecode(wstring text)
{
	wstring decoded = L"";
	wchar_t temp[] = L"0x00";
	size_t len = text.length();
	int sequence = 0;
	wchar_t conwch = 0;
	for(size_t i = 0; i < len; i++)
	{	
		wchar_t wch = text.at(i++);
		if((wch == '%') && (i+1 < len))
		{			
			temp[2] = text.at(i++);
			temp[3] = text.at(i);
			long tconwch = wcstol(temp, NULL, 16);
			if(tconwch <= 0x7F) {
				decoded += tconwch; // normal ascii char
			} else if(tconwch >= 0x80 && tconwch <= 0xBF) { // partial byte
				tconwch = tconwch & 0x3F;
				if(sequence-- == 2)
					tconwch = tconwch << 6;
				conwch |= tconwch;
				if(sequence == 0)
					decoded += conwch;
			} else if(tconwch >= 0xC0 && tconwch <= 0xDF) {
				conwch = (tconwch & 0x1F) << 6; // make space for partial bytes
				sequence = 1; // 1 more partial bytes follow
			} else if(tconwch >= 0xE0 && tconwch <= 0xEF) {
				conwch = (tconwch & 0xF) << 12; // make space for partial bytes
				sequence = 2; // 2 more partial bytes follow
			}
		} else {
			decoded += text.at(--i);
		}
	}
	return decoded;
}


void DebugPrint(LPCTSTR pszFormat, ... )
{
//#ifdef _DEBUG
	
    wchar_t szOutput[MAX_PATH * 2];
	va_list argList;
	va_start(argList, pszFormat);
	StringCchVPrintf(szOutput, MAX_PATH*2, pszFormat, argList);
	va_end(argList);
	//printf("%ls\n", szOutput);
	OutputDebugString(szOutput);
	
//#endif
}

void decode_base64(char* encodedBuffer)
{
	size_t position = 0;
	char encoded[4];
	size_t len = strlen(encodedBuffer);
	for(size_t i = 0; i < len+1; i++)
	{
		if((i > 0) && ((i % 4) == 0))
		{
			encodedBuffer[position++] = (unsigned char) (b64_index[encoded[0]] << 2 | b64_index[encoded[1]] >> 4);
			encodedBuffer[position++] = (unsigned char) (b64_index[encoded[1]] << 4 | b64_index[encoded[2]] >> 2);	
			encodedBuffer[position++] = (unsigned char) (b64_index[encoded[2]] << 6 | b64_index[encoded[3]]);
		}
		encoded[i%4] = encodedBuffer[i];
	}
	/* Should alway succeed as base64 encoded string length > decoded string length */
	if(position < len)
	{
		encodedBuffer[position] = '\0';
	}
}

char* encode_base64(char* cleartextBuffer, unsigned int length, size_t* encodedLength)
{
	/* Fairly ineffecient base64 encoding method ... could be a lot better but it works
	   at the moment. If performance is slow when transferring large files this could be
	   the problem */
	unsigned int len = length;	
	int nBlocks = len/3;
	unsigned int remainder = len % 3;
	int position = 0;
	if(remainder != 0)
	{
		nBlocks++;
	}
	char* encodedBuffer = (char*)malloc((nBlocks*4)+2);

	int k = 0;
	for(unsigned int i = 0; i < len; i+=3)
	{
		unsigned int block = 0;

		block |= ((unsigned char)cleartextBuffer[i] << 16);
		if(i+1 < len)
			block |= ((unsigned char)cleartextBuffer[i+1] << 8);
		if(i+2 < len)
			block |= ((unsigned char)cleartextBuffer[i+2]);

		encodedBuffer[k++] = b64_list[(block >> 18) & 0x3F];
		encodedBuffer[k++] = b64_list[(block >> 12) & 0x3F];
		encodedBuffer[k++] = b64_list[(block >> 6) & 0x3F];
		encodedBuffer[k++] = b64_list[(block & 0x3F)];
	}

	if(remainder > 0)
	{
		for(unsigned int i = remainder; i < 3; i++)
		{
			encodedBuffer[k-(3-i)] = '=';
		}
	}
	*encodedLength = k;
	return encodedBuffer;
}

size_t convertTimefieldsToString(TIME_FIELDS time, wchar_t* buffer, size_t bufferLength)
{
	wchar_t szTime[16];
	wchar_t wtime[256];
	ZeroMemory(&szTime, sizeof(szTime));
	ZeroMemory(&wtime, sizeof(wtime));
	_itow_s(time.wDay,szTime,16,10);
	wcscat_s(wtime, 256, szTime);
	wcscat_s(wtime, 256, L"/");
	_itow_s(time.wMonth,szTime,16,10);
	wcscat_s(wtime, 256, szTime);
	wcscat_s(wtime, 256, L"/");
	_itow_s(time.wYear,szTime,16,10);
	wcscat_s(wtime, 256, szTime);
	wcscat_s(wtime, 256, L" ");
	_itow_s(time.wHour,szTime,16,10);
	wcscat_s(wtime, 256, szTime);
	wcscat_s(wtime, 256, L":");
	_itow_s(time.wMinute,szTime,16,10);
	wcscat_s(wtime, 256, szTime);
	wcscat_s(wtime, 256, L":");
	_itow_s(time.wSecond,szTime,16,10);
	wcscat_s(wtime, 256, szTime);
	wcscat_s(wtime, 256, L".");
	_itow_s(time.wMilliseconds,szTime,16,10);
	wcscat_s(wtime, 256, szTime);
	size_t timeLength = wcslen(wtime);
	if(bufferLength >= timeLength)
	{
		wcscpy_s(buffer, bufferLength, wtime);
		return timeLength;
	} else {
		return -1;
	}
}