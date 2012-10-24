/*
 * klz.cpp  联众帐号偷窥器。 based on oldmonitor/packet.cpp#GetLZAccount
 *
 * Copyright 2007 - 2009 	kingstone,Inc
 *
 *
 * This modules is NOT open source. You can not even see its source code.
 * We shipped this file just make it easier for building faster executable.
 *
 */

#include <string.h>
#include <stdlib.h>

#include <net/ethernet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>

#include <gmodule.h>

#include "pcap_hander.h"

#define MAX_LZ_PORT 3999
#define MIN_LZ_PORT 2000


static gboolean CheckAccout(char *pAccount, int nLen)
{
	for (int n=0;n<nLen;n++)
	{
		if (pAccount[n]<='z'&&pAccount[n]>='a')
		{
			continue;
		}
		if (pAccount[n]<='Z'&&pAccount[n]>='A')
		{
			continue;
		}
		if (pAccount[n]<='9'&&pAccount[n]>='0')
		{
			continue;
		}
		return 0;
	}
	return 1;
}
#if 0
static int RecordLZ(char*LZ,u_char*packet,in_addr_t ip)
{
	struct NetAcount na(NetAcountType_LZ,packet);
	na.dstip = ip;
	na.data = LZ;
	na.ip = ((struct iphdr*)(packet + ETH_HLEN))->saddr;
	strcpy(na.strType,"2001");
	RecordAccout(&na);
	return 1;
}
#endif
static gboolean GetLZAccount(pcap_process_thread_param * param , gpointer user_data)
{
    struct iphdr * ip_head = (struct iphdr*)  param->packet_ip_contents;
    /**************************************************
     *TCP头也就在IP头的后面。IP头的大小为20字节，不过最好从头里读取
     **************************************************/
    struct tcphdr * tcp_head = (struct tcphdr*) (param->packet_ip_contents+ ip_head->ihl * 4);
    /**************************************************
     *TCP数据现对于tcp头的偏移由doff给出。这个也是tcp头的大小**
     **************************************************/
    char* tcpdata = (char*) tcp_head + tcp_head->doff * 4;
    int tcpdatelen = ntohs(ip_head->tot_len) - tcp_head->doff * 4 - ip_head->ihl * 4;
    /*太小的包肯定就不是*/
    if (tcpdatelen < 12)return 0;


	const int nSize=80;
	char strLZ[nSize];

	memset(strLZ,0,nSize);

	short dport = ntohs(tcp_head->dest);

	if( dport >= MIN_LZ_PORT && dport<=MAX_LZ_PORT)
	{
		char str1[]={0x30,1,0,0};
		char str2[]={0x31,0,0,0};
		if( memcmp(tcpdata,str1,4)==0 )
		{
			char *pName= tcpdata+12;
			int nLen=strlen(pName);
			if (nLen>nSize-1)
				return 0;
			strcpy(strLZ,pName);
			if (!CheckAccout(strLZ,strlen(strLZ)))
			{
				return 0;
			}
			return 1;
		}
		else if( memcmp(tcpdata,str2,4)==0 )
		{
			char *pName=tcpdata+40;
			int nLen=strlen(pName);
			if (nLen>nSize-1)
				return 0;
			strcpy(strLZ,pName);
			if (!CheckAccout(strLZ,strlen(strLZ)))
			{
				return 0;
			}
			return 1;
		}
	}
	return 0;
}

static void * hander;
G_MODULE_EXPORT gchar * g_module_check_init(GModule *module)
{
	hander = pcap_hander_register(GetLZAccount, 0, IPPROTO_TCP,0);
	return 0;
}

G_MODULE_EXPORT void g_module_unload(GModule*module)
{
	pcap_hander_unregister(hander);
}

