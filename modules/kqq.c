/*
 *      qq.cpp
 *
 *      Copyright 2009 microcai <microcai@microcai.com>
 *
 *      This program is non-free software; you can not redistribute it
 *      and/or modify it.
 *
 *      This program is distributed as a plugin for monitor.
 *      This plugin is Copyrighted! It contains my company's hard work.
 *      QQ is so hard too analyse! You should respect our work.
 */

#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <stdio.h>
#include <string.h>
#include <gmodule.h>
#include <pthread.h>

#include "pcap_hander.h"
#include "utils.h"

//std::map<std::string, time_t> qq_time_map;

#define QQ_DPORT  0x401F //8000
#define QQ_HTTPDPORT  0x5000 //80
#define QQ_VIPDPORT  0xBB01 //443
#define QQ_SPORT1 0x8813 //5000
#define QQ_SPORT2 0xA00F //4000


static const char * Type_QQ = "1002";

static int record_QQ_number(u_int qq, in_addr_t ip,const u_char*packet)
{
	//syslog(LOG_NOTICE,"QQ number is : %u\n",qq);

	static pthread_mutex_t lock=PTHREAD_MUTEX_INITIALIZER;

	char qqnum[80];
	sprintf(qqnum, "%u", qq);

#if 0
	time_t tmNow = time(NULL);
	pthread_mutex_lock(&lock);
	std::map<std::string, time_t>::iterator it = qq_time_map.begin();
	for (; it != qq_time_map.end();)
	{
		if ((tmNow - it->second) > 120)
			qq_time_map.erase(it++);
		else
			++it;
	}
	if (qq_time_map.find(qqnum) != qq_time_map.end())
	{
		pthread_mutex_unlock(&lock);
		return 0;
	}
	qq_time_map[qqnum] = tmNow;
	pthread_mutex_unlock(&lock);
#endif

	struct tcphdr* tcp = (struct tcphdr*)(packet + 14 + sizeof(struct iphdr));

	RecordAccout(Type_QQ,ip,* ( in_addr_t *) (packet +  28),packet + 6 ,"",qqnum, "",ntohs(tcp->dest));

    return 1;

}

static int qq_packet_callback ( struct pcap_pkthdr * hdr ,  const guchar  * packet , gpointer user_data )
{
	u_int	iQQnum=0;
	u_char *pQQNumber = ( u_char* ) &iQQnum ;

	struct iphdr * ip_head = ( struct iphdr* ) ( packet + 14 );

	if ( ip_head->protocol ==IPPROTO_UDP )
	{
		struct udphdr *udphead = ( struct udphdr * ) ( ( char* ) ip_head + ip_head->ihl*4 );
		u_char * udp_packet = ( u_char* ) udphead + sizeof ( struct udphdr );

		if ( udphead->len <=5 ) return 0;

		switch ( udphead->dest )
		{
			case QQ_DPORT:
				if ( ( udp_packet[0] == 0x02 ) && ( udp_packet[3] == 0x00 ) && ( udp_packet[4] == 0x62||udp_packet[4] == 0x22||udp_packet[4] == 0x91 ) )
				{
					for ( int i=0; i<4; i++ )
					{
						pQQNumber[3-i] =  udp_packet[7+i];
					}
					return record_QQ_number ( iQQnum , ip_head->saddr,packet);
				}
		}
	}
	else if ( ip_head->protocol == IPPROTO_TCP )
	{
		struct tcphdr *tcphead = ( struct tcphdr* ) ( ( char* ) ip_head + ip_head->ihl*4 );
		u_char*tcpdata = ( u_char* ) ( tcphead + tcphead->doff*4 );
		int tcpdataLen = ntohs(ip_head->tot_len) - ip_head->ihl*4 -  tcphead->doff*4;
		if ( tcpdataLen < 5 )
			return 0;
		switch ( tcphead->dest )
		{
			case QQ_HTTPDPORT:
				if ( tcpdata[0] == 0x00 && tcpdata[2] == 0x02
				        && ( tcpdata[3] == 0x0c || tcpdata[3] == 0x0d ) )
				{
					for ( int i=0; i<4; i++ )
					{
						pQQNumber[3-i] = tcpdata[9+i];
					}
					return record_QQ_number ( iQQnum, ip_head->saddr ,packet);
				}
				break;
			case QQ_VIPDPORT:
				if ( tcpdata[2] == 0x02 && tcpdata[5] == 0x00&&tcpdata[6] == 0x22 )
				{
					for ( int i=0; i<4; i++ )
					{
						pQQNumber[3-i] = tcpdata[9+i];
					}
					return record_QQ_number ( iQQnum , ip_head->saddr,packet);
				}
				break;
		}
	}
	return 0;
}

static void* protohander[3];
static void* base_addr;

G_MODULE_EXPORT gchar * g_module_check_init(GModule *module)
{
	protohander[1] = pcap_hander_register(qq_packet_callback,QQ_HTTPDPORT,IPPROTO_TCP , 0 );
	protohander[2] = pcap_hander_register ( qq_packet_callback,QQ_VIPDPORT,IPPROTO_TCP,0 );

	protohander[0] = pcap_hander_register ( qq_packet_callback,QQ_DPORT ,IPPROTO_UDP,0 );
    return 0;
}

G_MODULE_EXPORT void g_module_unload()
{
	// here, we need to
	for ( int i=0; i< 3;++i )
		pcap_hander_unregister( protohander[i] );
}


char module_name[]="QQ号码分析";
