#include "NetworkAdapter.h"

NetworkAdapter::NetworkAdapter(NetworkPacketDumper* npDumper, string aName, pcap_t* adap)
{
	networkPacketDumper = npDumper;
	adapterName = aName;
	adapter = adap;
	running = false;


	
}

NetworkAdapter::~NetworkAdapter(void)
{
	stop();
}

void
NetworkAdapter::start()
{
	if(!running)
	{
		string logName = "logs\\";
		logName += adapterName.substr(adapterName.find_last_of("\\")+1);
		logName += ".pcap";
			
		dumpFile = networkPacketDumper->pfn_pcap_dump_open(adapter, logName.c_str());
		adapterThread = new Thread(this);
		string threadName = "NetworkPacketDumper-";
		threadName += adapterName;
		char* t = (char*)threadName.c_str();
		adapterThread->start(t);
		running = true;
	}
}

void
NetworkAdapter::stop()
{
	if(running)
	{
		adapterThread->stop();
		networkPacketDumper->pfn_pcap_dump_close(dumpFile);
		networkPacketDumper->pfn_pcap_close(adapter);
		running = false;
	}
}

void
NetworkAdapter::run()
{
	int res;
	struct pcap_pkthdr *header;
	const u_char *pkt_data;
	while((res = networkPacketDumper->pfn_pcap_next_ex( adapter, &header, &pkt_data)) >= 0)
	{     
        if(res > 0)
		{
			networkPacketDumper->pfn_pcap_dump((unsigned char *) dumpFile, header, pkt_data);
		}
	}	
}
