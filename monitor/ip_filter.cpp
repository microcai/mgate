/*
 *      ip_filter.c
 *
 *      Copyright 2009 MicroCai <microcai@sina.com>
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *      MA 02110-1301, USA.
 */
#ifdef DEBUG
	#include <iostream>
#endif
#include <iostream>
#include <sys/socket.h>
#include <net/ethernet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <pcap.h>
#include <libnet.h>

#include "libmicrocai.h"
#include "protocol_def.h"

static u_int32_t net_ip;
static u_int32_t net_mask;

static void	ip_filter_callback( u_char *userdata, const struct pcap_pkthdr * pkghdr,
                         const u_char *packet_content )
{

    struct iphdr* ip_head =( struct iphdr* )(packet_content + ETH_HLEN);
    u_int16_t   port;
    int i;
    PROTOCOL_HANDLER * handlerlist;

    //non TCP or UDP is ignored
    if( ip_head->protocol!=IPPROTO_TCP&&ip_head->protocol!=IPPROTO_UDP)
        return;

    //out -> in is ignored
    if( (ip_head->saddr & net_mask )!= (net_ip & net_mask))
    {
    	return;
    }

    //local communication is ignored
    if((ip_head->daddr & net_mask) == (net_ip & net_mask)) return;

    if( is_client_online( (char*)(packet_content + ETH_ALEN), ip_head->saddr)==0)
    {
#ifndef DEBUG
    	if(ip_head->protocol == IPPROTO_TCP)
    		return redirect_to_local_http( net_ip, packet_content, ip_head );
    	return ;
#endif
    }


    /*********************************************************************
     * for both UDP and TCP protocols ,source and destination port is
	 * just behind the IP header, and destination port is just behind
	 * the source port
	 *********************************************************************/
	port = *((u_int16_t*) (packet_content + ETH_HLEN + ip_head->ihl * 4 + 2));
	//here we get a list of handler;
	handlerlist = (PROTOCOL_HANDLER*) get_registerd_handler(port,
			ip_head->protocol);
	//then we call these handler one by one
	if (handlerlist)
	{
		i = 0;
		while(handlerlist[i])
		{
			/*if a handler can process the packet, then we just finished
			 * otherwise, we need to call another one.*/
			if ((handlerlist[i])(0, (u_char*) packet_content))
				break;
			i++;
		}
		// free the list in case memory leak
		free(handlerlist);
	}
}

extern void pcap_thread_func(struct pcap_thread_args *arg)
{
	char error_content[PCAP_ERRBUF_SIZE];
	struct bpf_program bpf_filter;

	pcap_t* pcap_handle;

	net_mask = arg->mask;
	net_ip = arg->ip;

	pcap_handle = pcap_open_live(arg->eth, BUFSIZ, 0, 0, error_content);
	if (!pcap_handle)
	{
		log_printf(L_ERROR, "无法打开 %s 抓 %s 数据包,请以root运行\n",arg->eth,
				arg->bpf_filter_string);
		return ;
	}


#ifdef DEBUG
	struct in_addr p;
	p.s_addr = net_ip;
	log_printf(L_DEBUG_OUTPUT,"listen on device %s and ip address is %s\n",arg->eth,inet_ntoa(p));
#endif

	pcap_compile(pcap_handle,&bpf_filter,arg->bpf_filter_string,0,net_ip);

	pcap_setfilter(pcap_handle, &bpf_filter);

	pcap_freecode(&bpf_filter);

	if (pcap_datalink(pcap_handle) != DLT_EN10MB)
	{
		log_printf(L_ERROR,"ERROR:%s is not an ethernet adapter\n",arg->eth);
#ifdef DEBUG
		return;
#else
		exit(1);
#endif
	}
	pcap_loop(pcap_handle,-1,ip_filter_callback,0);
	pcap_close(pcap_handle);
}
