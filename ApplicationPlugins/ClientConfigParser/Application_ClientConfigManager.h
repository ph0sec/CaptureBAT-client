/*
 *	PROJECT: Capture
 *	FILE: Application_ClientConfigManager.h
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

#include "..\..\ApplicationPlugin.h"
#include "FileDownloader.h"

#define _WIN32_WINNT 0x0501
#include <string>
#include <list>
#include <iostream>
#include <fstream>
#include <vector>
#include <hash_map>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/find_iterator.hpp>
#include <boost/algorithm/string/finder.hpp> 
#include <boost/tokenizer.hpp>

/* Define the application plugin name and the application names it supports */
#define APPLICATION_PLUGIN_NAME		Application_ClientConfigManager

using namespace std;
using namespace boost;

typedef struct _APPLICATION
{
	wstring name;
	wstring path;
	bool downloadURL;
} APPLICATION, *PAPPLICATION;

class Application_ClientConfigManager : public ApplicationPlugin
{
public:
	typedef split_iterator<wstring::iterator> sf_it;
	typedef pair <wstring, PAPPLICATION> ApplicationPair;
public:
	Application_ClientConfigManager(void);
	~Application_ClientConfigManager(void);
	
	DWORD visitUrl(Url* url,DWORD* minorError);
	wchar_t** getSupportedApplicationNames()
	{
		
		return supportedApplications;
	}
	unsigned int getPriority() { return 0; };
private:
	void loadApplicationsList();
	static BOOL CALLBACK EnumWindowsProc(HWND hwnd,LPARAM lParam);
	bool closeProcess(DWORD processId, DWORD* error);

	wchar_t** supportedApplications;
	stdext::hash_map<wstring, APPLICATION*> applicationsMap;
};

/* DO NOT REMOVE OR MOVE THIS LINE ... ADDS EXPORTS (NEW/DELETE) TO THE PLUGIN */
/* Must be included after the actual plugin class definition */
#include "..\..\PluginExports.h"
