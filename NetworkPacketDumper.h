/*
 *	PROJECT: Capture
 *	FILE: NetworkPacketDumper.h
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
#include <pcap.h>

#include <string>
#include <hash_map>
#include "Thread.h"
#include "Monitor.h"
#include "NetworkAdapter.h"

using namespace std;

class NetworkAdapter;

/* 
	Class: NetworkPacketDumper
	
	Interfaces with winpcap and allows packets to be dumped to a file when they are
	sent or received. In the constructor it finds all available network adapters
	and created <NetworkAdapter> opbjects which hold the necessary information for them
	to dump the actual packets. This object just manages the NetworkAdapter objects and
	provides an interface to pcap.dll for them to access.

	Underlying, this is really a monitor so can load kernel drivers and have an exclusion
	list however this is not needed at the moment. Future uses could be acutally tracking
	socket connections etc.
*/
class NetworkPacketDumper : public Monitor
{
public:
	/* Dynamically link to pcap.dll. Workaround so that we can detect if WinPCAP
	   is actually installed on the system */
	typedef int (*pcap_findalldevs_c)(pcap_if_t **, char *);
	typedef pcap_t	*(*pcap_open_live_c)(const char *, int, int, int, char *);
	typedef void	(*pcap_freealldevs_c)(pcap_if_t *);
	typedef void	(*pcap_close_c)(pcap_t *);
	typedef pcap_dumper_t *(*pcap_dump_open_c)(pcap_t *, const char *);
	typedef void	(*pcap_dump_close_c)(pcap_dumper_t *);
	typedef int 	(*pcap_next_ex_c)(pcap_t *, struct pcap_pkthdr **, const u_char **);
	typedef void	(*pcap_dump_c)(u_char *, const struct pcap_pkthdr *, const u_char *);

	pcap_findalldevs_c pfn_pcap_findalldevs;
	pcap_open_live_c pfn_pcap_open_live;
	pcap_freealldevs_c pfn_pcap_freealldevs;
	pcap_close_c pfn_pcap_close;
	pcap_dump_open_c pfn_pcap_dump_open;
	pcap_dump_close_c pfn_pcap_dump_close;
	pcap_next_ex_c pfn_pcap_next_ex;
	pcap_dump_c pfn_pcap_dump;

	typedef pair<string, pcap_t*> DevicePair;
	typedef pair<string, pcap_dumper_t*> DeviceLogPair;
public:
	NetworkPacketDumper(void);
	~NetworkPacketDumper(void);

	void start();
	void stop();

	bool isMonitorRunning() { return monitorRunning; }
	bool isDriverInstalled() { return driverInstalled; }

private:
	bool monitorRunning;
	bool driverInstalled;

	std::list<NetworkAdapter*> adapterList;
	HMODULE hModWinPcap;
};
