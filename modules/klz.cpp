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

#include <iostream>
#include <string.h>
#include <stdlib.h>

#include <net/ethernet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>

#include "libdreamtop.h"


#define MAX_LZ_PORT 3999
#define MIN_LZ_PORT 2000


static bool CheckAccout(char *pAccount, int nLen)
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
static int GetLZAccount(struct so_data*,u_char *packet)
{
    /**************************************************
     *IP数据包通常就在以太网数据头的后面。以太网头的大小为14字节*
     **************************************************/
    struct iphdr * ip_head = (struct iphdr*) (packet + ETH_HLEN);
    /**************************************************
     *TCP头也就在IP头的后面。IP头的大小为20字节，不过最好从头里读取
     **************************************************/
    struct tcphdr * tcp_head = (struct tcphdr*) (packet + ETH_HLEN + ip_head->ihl * 4);
    /**************************************************
     *TCP数据现对于tcp头的偏移由doff给出。这个也是tcp头的大小**
     **************************************************/
    char* tcpdata = (char*) tcp_head + tcp_head->doff * 4;
    int tcpdatelen = ntohs(ip_head->tot_len) - tcp_head->doff * 4 - ip_head->ihl * 4;
    /*太小的包肯定就不是*/
    if (tcpdatelen < 12)return 0;


	const int nSize=80;
	char strLZ[nSize]={0};
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
extern "C" int __module_init(struct so_data*so)
{
	hander = register_protocol_handler(GetLZAccount, 0, IPPROTO_TCP);
	return 0;
}

extern "C" int	so_can_unload(  )
{
	return 1;
}

static void __attribute__((destructor)) so__unload(void)
{
	un_register_protocol_handler(hander);
	sleep(4);
}

char module_name[]="联众帐号监视";

