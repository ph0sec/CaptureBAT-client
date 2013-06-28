/*
 *	PROJECT: Capture
 *	FILE: CaptureGlobal.h
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
#define _WIN32_WINNT 0x0501
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
//#define LOCALTEST			// Output some local statistics for debugging

/*
	This header is included by all objects in the client. This allows all object
	to have access to the various singleton objects in the project
*/
#include "ErrorCodes.h"
#include <windows.h>
#include "ProcessManager.h"
#include "EventController.h"
#include "Logger.h"
#include "OptionsManager.h"

static const wchar_t * hexenc[] = {
    L"%00", L"%01", L"%02", L"%03", L"%04", L"%05", L"%06", L"%07",
    L"%08", L"%09", L"%0a", L"%0b", L"%0c", L"%0d", L"%0e", L"%0f",
    L"%10", L"%11", L"%12", L"%13", L"%14", L"%15", L"%16", L"%17",
    L"%18", L"%19", L"%1a", L"%1b", L"%1c", L"%1d", L"%1e", L"%1f",
    L"%20", L"%21", L"%22", L"%23", L"%24", L"%25", L"%26", L"%27",
    L"%28", L"%29", L"%2a", L"%2b", L"%2c", L"%2d", L"%2e", L"%2f",
    L"%30", L"%31", L"%32", L"%33", L"%34", L"%35", L"%36", L"%37",
    L"%38", L"%39", L"%3a", L"%3b", L"%3c", L"%3d", L"%3e", L"%3f",
    L"%40", L"%41", L"%42", L"%43", L"%44", L"%45", L"%46", L"%47",
    L"%48", L"%49", L"%4a", L"%4b", L"%4c", L"%4d", L"%4e", L"%4f",
    L"%50", L"%51", L"%52", L"%53", L"%54", L"%55", L"%56", L"%57",
    L"%58", L"%59", L"%5a", L"%5b", L"%5c", L"%5d", L"%5e", L"%5f",
    L"%60", L"%61", L"%62", L"%63", L"%64", L"%65", L"%66", L"%67",
    L"%68", L"%69", L"%6a", L"%6b", L"%6c", L"%6d", L"%6e", L"%6f",
    L"%70", L"%71", L"%72", L"%73", L"%74", L"%75", L"%76", L"%77",
    L"%78", L"%79", L"%7a", L"%7b", L"%7c", L"%7d", L"%7e", L"%7f",
    L"%80", L"%81", L"%82", L"%83", L"%84", L"%85", L"%86", L"%87",
    L"%88", L"%89", L"%8a", L"%8b", L"%8c", L"%8d", L"%8e", L"%8f",
    L"%90", L"%91", L"%92", L"%93", L"%94", L"%95", L"%96", L"%97",
    L"%98", L"%99", L"%9a", L"%9b", L"%9c", L"%9d", L"%9e", L"%9f",
    L"%a0", L"%a1", L"%a2", L"%a3", L"%a4", L"%a5", L"%a6", L"%a7",
    L"%a8", L"%a9", L"%aa", L"%ab", L"%ac", L"%ad", L"%ae", L"%af",
    L"%b0", L"%b1", L"%b2", L"%b3", L"%b4", L"%b5", L"%b6", L"%b7",
    L"%b8", L"%b9", L"%ba", L"%bb", L"%bc", L"%bd", L"%be", L"%bf",
    L"%c0", L"%c1", L"%c2", L"%c3", L"%c4", L"%c5", L"%c6", L"%c7",
    L"%c8", L"%c9", L"%ca", L"%cb", L"%cc", L"%cd", L"%ce", L"%cf",
    L"%d0", L"%d1", L"%d2", L"%d3", L"%d4", L"%d5", L"%d6", L"%d7",
    L"%d8", L"%d9", L"%da", L"%db", L"%dc", L"%dd", L"%de", L"%df",
    L"%e0", L"%e1", L"%e2", L"%e3", L"%e4", L"%e5", L"%e6", L"%e7",
    L"%e8", L"%e9", L"%ea", L"%eb", L"%ec", L"%ed", L"%ee", L"%ef",
    L"%f0", L"%f1", L"%f2", L"%f3", L"%f4", L"%f5", L"%f6", L"%f7",
    L"%f8", L"%f9", L"%fa", L"%fb", L"%fc", L"%fd", L"%fe", L"%ff"
	};


class CaptureGlobal
{
public:
	static wstring urlEncode(wstring text);
	static wstring urlDecode(wstring text);
};

typedef struct _TIME_FIELDS {
    WORD wYear;
    WORD wMonth;
    WORD wDay;
    WORD wHour;
    WORD wMinute;
    WORD wSecond;
    WORD wMilliseconds;
    WORD wWeekday;
} TIME_FIELDS;

/*
	Various C methods to allow base64 encoding and decoding
*/
extern "C" {

void DebugPrint(LPCTSTR pszFormat, ... );

static char b64_list[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

#define XX 100

static int b64_index[256] = {
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,62, XX,XX,XX,63,
    52,53,54,55, 56,57,58,59, 60,61,XX,XX, XX,XX,XX,XX,
    XX, 0, 1, 2,  3, 4, 5, 6,  7, 8, 9,10, 11,12,13,14,
    15,16,17,18, 19,20,21,22, 23,24,25,XX, XX,XX,XX,XX,
    XX,26,27,28, 29,30,31,32, 33,34,35,36, 37,38,39,40,
    41,42,43,44, 45,46,47,48, 49,50,51,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
};

void decode_base64(char* encodedBuffer);

char* encode_base64(char* cleartextBuffer, size_t length, size_t* encodedLength);

size_t convertTimefieldsToString(TIME_FIELDS time, wchar_t* buffer, size_t bufferLength);
};