/*
 *	PROJECT: Capture
 *	FILE: Visitor.h
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
#include <string>
#include <queue>
#include <list>
#include <iostream>
#include <fstream>
#include <vector>
#include <hash_map>
#include <boost/signal.hpp>
#include <boost/bind.hpp>
#include <boost\regex.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/find_iterator.hpp>
#include <boost/algorithm/string/finder.hpp> 
#include <boost/lexical_cast.hpp>
#include <boost/tokenizer.hpp>
#include "Url.h"
#include "Thread.h"
#include "ApplicationPlugin.h"

using namespace std;
using namespace boost;

typedef split_iterator<string::iterator> sf_it;

/*
	Class: Visitor

	Listens for visit events from the <EventController>. When one is received, 
	it finds the url requested to visit and which client to use, and then proceeds 
	to visit the url by creating the process requested and instructing it to visit 
	the url.
*/
class Visitor : public Runnable
{
public:
	typedef boost::signal<void (DWORD, DWORD, wstring, wstring)> signal_visitEvent;
	typedef pair <HMODULE, std::list<ApplicationPlugin*>*> PluginPair;
	typedef pair <wstring, ApplicationPlugin*> ApplicationPair;
	typedef pair <ApplicationPlugin*, Url*> VisitPair;
public:
	Visitor(void);
	~Visitor(void);

	boost::signals::connection onVisitEvent(const signal_visitEvent::slot_type& s);
private:
	void loadClientPlugins();
	void unloadClientPlugins();

	void run();

	void onServerEvent(Element* pElement);

	ApplicationPlugin* createApplicationPluginObject(HMODULE hPlugin);

	HANDLE hQueueNotEmpty;
	bool visiting;
	queue<VisitPair> toVisit;
	Thread* visitorThread;
	signal_visitEvent signalVisitEvent;
	stdext::hash_map<HMODULE, std::list<ApplicationPlugin*>*> applicationPlugins;
	boost::signals::connection onServerVisitEventConnection;
	stdext::hash_map<wstring, ApplicationPlugin*> applicationMap;
};
