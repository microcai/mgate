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

#include <iostream>
#include <sys/socket.h>
#include <net/ethernet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <pcap.h>
#include <libnet.h>
#include <stdio.h>
#include "libmicrocai.h"
#include "protocol_def.h"

//
//static void mac2mac(const u_char * d,char *strmac)
//{
//	snprintf(strmac,18,"%02x:%02x:%02x:%02x:%02x:%02x", d[0], d[1], d[2], d[3], d[4], d[5]);
//}
//
//static void ntoa(char * ip, in_addr_t pp)
//{
//	u_char* p= (u_char*)&pp;
//	snprintf(ip,16,"%d.%d.%d.%d", p[0],p[1],p[2],p[3]);
//}
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

extern void *pcap_thread_func(struct pcap_thread_args *arg)
{
	u_int32_t	net_ip;
	u_int32_t	net_mask;
	pcap_t *	pcap_handle;
	u_char		mac_addr[6];

	pcap_pkthdr *pcaphdr;
	const u_char*packet_contents;

	struct iphdr *ip_head;
    u_int16_t   port;
    int			i;
    PROTOCOL_HANDLER handlerlist[1024];

    pcap_handle = arg->pcap_handle;
	net_mask = arg->mask;
	net_ip = arg->ip;
	memcpy(mac_addr,arg->mac_addr,6);

	u_char *packet_content;

	packet_content = (typeof(packet_content))malloc(ETHER_MAX_LEN);


	for(;;)
	{
		pthread_mutex_lock(&lock);
		while(pcap_next_ex(pcap_handle, &pcaphdr, &packet_contents) != 1);
		memcpy(packet_content,packet_contents,pcaphdr->len);
		pthread_mutex_unlock(&lock);

	    ip_head =( typeof(ip_head) )(packet_content + ETH_HLEN);

//	    //non TCP or UDP is ignored
	    if( ip_head->protocol!=IPPROTO_TCP&&ip_head->protocol!=IPPROTO_UDP)
	        continue;

	    //out -> in is ignored
	    if( (ip_head->saddr & net_mask )!= (net_ip & net_mask))
	    {
	    	continue;
	    }

	    //local communication is ignored
	    if((ip_head->daddr & net_mask) == (net_ip & net_mask)) continue;
	    if(ip_head->saddr == net_ip ) continue;

	    /*********************************************************************
	     * for both UDP and TCP protocols ,source and destination port is
		 * just behind the IP header, and destination port is just behind
		 * the source port
		 *********************************************************************/
	    port = *((u_int16_t*) (packet_content + ETH_HLEN + ip_head->ihl * 4 + 2));

	    if( mac_is_alowed( (u_char*)packet_content + ETH_ALEN , ip_head->saddr)==false)
	    {

	    	if(ip_head->protocol == IPPROTO_TCP )
	    		redirect_to_local_http( net_ip, packet_content, ip_head );
	    	continue;
	    }

		//here we get a list of handler;
	    bzero(handlerlist,sizeof(handlerlist));
		get_registerd_handler(handlerlist, 1024, port, ip_head->protocol);
		//then we call these handler one by one
		i = 0;
		while (handlerlist[i])
		{
			/*if a handler can process the packet, then we just finished
			 * otherwise, we need to call another one.*/
			if ((handlerlist[i])(0, (u_char*) packet_content))
				break;
			i++;
		}
	}
	pcap_close(arg->pcap_handle);
	return 0;
}
