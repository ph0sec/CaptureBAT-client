/*
 *	PROJECT: Capture
 *	FILE: Permission.h
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
#include <list>
#include <string>
#include <boost\regex.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/find_iterator.hpp>
#include <boost/algorithm/string/finder.hpp> 
#include <boost/tokenizer.hpp> 

using namespace std;

typedef enum _PERMISSION_CLASSIFICATION {
    NO_MATCH,
	ALLOWED,
	DISALLOWED
} PERMISSION_CLASSIFICATION;

/*
	Class: Permission

	TODO should really be called an exclusion.

	Contains a subject such as a process, and and object which the subject is interacting
	with such as a file. If the 2 regex matches and the permission is allowed then the
	event is excluded.
*/
class Permission
{
public:
	/*
		Function: Check
	*/
	PERMISSION_CLASSIFICATION Check(wstring subject, wstring object);

	/*
		Variable: subjects
	*/
	std::list<boost::wregex> subjects;

	/*
		Variable: permissions
	*/
	std::list<boost::wregex> objects;

	/*
		Variable: allow
	*/
	bool allow;

	/*
		Variable: permaneant
		
		Whether or not this exclusion is permaneant or not. Can only be set
		by objects which create their own exclusions.
	*/
	bool permaneant;
};