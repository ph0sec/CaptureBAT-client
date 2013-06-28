/*
 *	PROJECT: Capture
 *	FILE: ApplicationPlugin.h
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
#include <windows.h>
#include <string>
#include <list>
#include "Url.h"
#include "ErrorCodes.h"

using namespace std;

class ApplicationPlugin
{
public:
	/* Visits a particular url. After visitation, this function needs to
	   return one of the status codes defined in ErrorCodes.h. Optionally
	   a minor error code can be placed into the minorError parameter. */
	virtual DWORD visitUrl(Url* url, DWORD* minorError) = 0;
	/* Returns a pointer to an array that is allocated in the plugin
	   and contains the list of supported application names */
	virtual wchar_t** getSupportedApplicationNames() = 0;
	/* Returns the priority of the plugin 0 .. 2^32. 0 being the lowest.
	   The priority is used so Capture can determine what plugin to use
	   if there are two which support the same application name */
	virtual unsigned int getPriority() = 0;

};
