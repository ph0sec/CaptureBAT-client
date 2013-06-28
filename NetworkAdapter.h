/*
 *	PROJECT: Capture
 *	FILE: NetworkAdapter.h
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
#include "Thread.h"
#include "Monitor.h"
#include <pcap.h>
#include <string>
#include "NetworkPacketDumper.h"
/*
	Class: NetworkAdapter

	Contains information about a particular network adapter on the system. When
	required it will start dumping packets sent and received on the adapter to
	a file.
*/

/* Gah ... A circular dependency */
class NetworkPacketDumper;

using namespace std;

class NetworkAdapter : public Runnable, Monitor
{
public:
	NetworkAdapter(NetworkPacketDumper* npDumper, string aName, pcap_t* adap);
	~NetworkAdapter(void);

	void start();
	void stop();
	void run();
private:
	bool running;
	string adapterName;
	pcap_t* adapter;
	NetworkPacketDumper* networkPacketDumper;
	pcap_dumper_t *dumpFile;
	Thread* adapterThread;
};
