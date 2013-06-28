/*
 *	PROJECT: Capture
 *	FILE: EventController.h
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
//#include "CaptureGlobal.h"
#include <boost/signal.hpp>
#include <boost/bind.hpp>
#include <vector>
#include <expatpplib.h>
#include <hash_map>

using namespace std;

typedef struct _ATTRIBUTE
{
	wstring name;
	wstring value;
} Attribute, * pAttribute;

typedef struct _ELEMENT
{
	wstring name;
	vector<Attribute> attributes;
	char * data;
	size_t dataLength;
} Element, * pElement;

/*
	Class: EventController

	The EventController is responsible for passing events to various objects in the
	client. It is a singleton and extends the expatpp SAX parser which allows the
	controller to parse XML documents. Objects listen for a particular event by
	calling connect_onServerEvent with a particular event type and a binding to
	a method on that object. This uses the boost signal slot functionality. Events can
	be sent to the controller in various ways. At the moment it is used to signal
	events from the server. For example when an Visit XML document is sent from the server
	the <Server> will receive it, and pass it to receiveServerEvent which will parse it
	to various XML Elements and based on the element name pass it to the various
	listeners.

	This is very similar to an event pump which received all external events and passes
	them to all of the internal objects. This can be extended very easy and was designed
	for the intention of creating a command line interface which could easily communicate
	with the internal object without knowing anything about them.
*/
class EventController : public expatpp
{
	
public:
	typedef boost::signal<void (Element*)> signal_serverEvent;
	typedef pair <wstring, signal_serverEvent*> OnServerEventPair;
public:
	~EventController(void);
	static EventController* getInstance();

	boost::signals::connection connect_onServerEvent(wstring eventType, const signal_serverEvent::slot_type& s);

	void receiveServerEvent(const char* xmlDocument);
private:
	static bool instanceCreated;
    static EventController *pEventController;
	EventController(void);

	void notifyListeners();

	stdext::hash_map<wstring, signal_serverEvent*> onServerEventMap;
	Element* pCurrentElement;
	
public:
	virtual void startElement(const XML_Char *name, const XML_Char **atts);
	virtual void endElement(const XML_Char* name);
	virtual void charData(const XML_Char *s, int len);
};
