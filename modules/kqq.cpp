
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

#include <iostream>
#include <string>

#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <stdio.h>
#include <string.h>

#include "libmicrocai.h"
#include "kmysql.h"
#include "my_log.h"

#define QQ_DPORT  0x401F //8000
#define QQ_HTTPDPORT  0x5000 //80
#define QQ_VIPDPORT  0xBB01 //443
#define QQ_SPORT1 0x8813 //5000
#define QQ_SPORT2 0xA00F //4000


std::string Type_QQ("1002");

static int record_QQ_number(u_int qq, in_addr_t ip,u_char*packet)
{
    std::cout << "QQ number is :" << qq << std::endl;

    struct NetAcount na(NetAcountType_QQ,packet);
    na.ip = ip;
    strcpy(na.strType, Type_QQ.c_str());
    char qqnum[80];

    sprintf(qqnum, "%u", qq);
    na.data = qqnum;

    //RecordAccout(&na);
    RecordAccout(&na);

    return 1;

}

static int qq_packet_callback ( struct so_data* sodata,u_char * packet )
{
	std::cout << "QQ caled" << std::endl;
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
				if ( ( udp_packet[0] == 0x02 ) && ( udp_packet[3] == 0x00 ) && ( udp_packet[4] == 0x22 ) )
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
		int tcpdataLen = ip_head->tot_len - ip_head->ihl*4 -  tcphead->doff*4;
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

extern "C" int __module_init(struct so_data*so)
{
	log_puts(L_OUTPUT,"\t注册QQ@TCP处理函数\n");
	protohander[1] = register_protocol_handler ( qq_packet_callback,QQ_HTTPDPORT,IPPROTO_TCP );
	protohander[2] = register_protocol_handler ( qq_packet_callback,QQ_VIPDPORT,IPPROTO_TCP );

	log_puts(L_OUTPUT,"\t注册QQ@UDP处理函数\n");

	protohander[0] = register_protocol_handler ( qq_packet_callback,QQ_DPORT ,IPPROTO_UDP );
	log_puts(L_OUTPUT,"QQ 分析模块loaded!\n");
	base_addr = so->module;
    return 0;
}
static void __attribute__ ( ( destructor ) ) so__unload ( void )
{
	for ( int i=0; i< 3;++i )
		un_register_protocol_handler ( protohander[i] );
}
char module_name[]="QQ号码分析";
