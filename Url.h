/*
 *	PROJECT: Capture
 *	FILE: Url.h
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
#include <string>

using namespace std;
/*
	Class: Url
	
	Object which holds the url information such as the actual URL and the visit time
*/
class Url
{
public:
	Url(wstring u, wstring app, int vTime);
	~Url(void);

	wstring getUrl() { return url; }
	wstring getApplicationName() { return applicationName; }
	int getVisitTime() { return visitTime; }

private:
	wstring url;
	wstring applicationName;
	int visitTime;
};
