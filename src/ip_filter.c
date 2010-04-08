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
#ifdef  HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/syslog.h>
#include <sys/socket.h>
#include <net/ethernet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <pcap.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <pcap/pcap.h>
#include <syslog.h>
#include <string.h>
#include <glib.h>

#ifdef HAVE_GETTEXT
#include <locale.h>
#include <libintl.h>
#define _(x) gettext(x)
#define N_(x) (x)
#endif

#include "libmicrocai-macros.h"
#include "libmicrocai-types.h"
#include "functions.h"

typedef struct _pcap_process_thread_param
{
	struct pcap_pkthdr pcaphdr;
	const u_char*packet_contents;
} pcap_process_thread_param;

static void pcap_process_thread_func(gpointer _thread_data, gpointer user_data)
{
	u_int16_t port;
	int i;
	PROTOCOL_HANDLER handlerlist[1024];

	pcap_process_thread_param * thread_data = _thread_data;
	//		recv(fno,packet_content,ETHER_MAX_LEN,0);

	const guchar * packet_content = thread_data->packet_contents;

	struct iphdr * ip_head = (typeof(ip_head))(packet_content + ETH_HLEN);

	/*********************************************************************
	 * for both UDP and TCP protocols ,source and destination port is
	 * just behind the IP header, and destination port is just behind
	 * the source port
	 *********************************************************************/
	port = *((u_int16_t*) (packet_content + ETH_HLEN + ip_head->ihl * 4 + 2));

#ifdef ENABLE_HOTEL
	if( mac_is_alowed_with_ip((u_char*)packet_content + ETH_ALEN , ip_head->saddr)==false)
	{

		if(ip_head->protocol == IPPROTO_TCP )
		redirect_to_local_http( net_ip, packet_content, ip_head );
		continue;
	}
#endif

	//here we get a list of handler;
	bzero(handlerlist, sizeof(handlerlist));
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
	g_free((void*)(thread_data->packet_contents));
	g_free(thread_data);
}

void *pcap_thread_func(void * thread_param)
{

	bpf_u_int32 ip, mask;

	GThreadPool * threadpool = g_thread_pool_new(pcap_process_thread_func, NULL, 80, TRUE, NULL);

	char errbuf[PCAP_ERRBUF_SIZE];
	struct bpf_program bpf_filter =
	{ 0 };

	pcap_t * pcap_handle = pcap_open_live("eth0", 65536, 0, 0, errbuf);
	if (!pcap_handle)
	{
		syslog(LOG_CRIT, _("ERROR:can not open %s for capturing!\n"), "eth0");
		closelog();
		return 0;
	}

	if (pcap_datalink(pcap_handle) != DLT_EN10MB)
	{
		syslog(LOG_CRIT, _("ERROR:%s is not an ethernet adapter\n"), "eth0");
		return 0;
	}

	char * net_interface = pcap_lookupdev(errbuf);

	pcap_lookupnet(net_interface, &ip, &mask, errbuf);

	pcap_compile(pcap_handle, &bpf_filter, "tcp or udp", 1, 0);

	pcap_setfilter(pcap_handle, &bpf_filter);

	pcap_freecode(&bpf_filter);

	struct pcap_pkthdr *pcaphdr;
	const u_char*packet_contents;

	pcap_process_thread_param * thread_data;

	for (;;)
	{

		pcap_next_ex(pcap_handle, &pcaphdr, &packet_contents);

		struct iphdr * ip_head = (typeof(ip_head))(packet_contents + ETH_HLEN);

		//	    //non TCP or UDP is ignored
		if (ip_head->protocol != IPPROTO_TCP && ip_head->protocol != IPPROTO_UDP)
			continue;

		//out -> in is ignored
		if ((ip_head->saddr & mask) != (ip & mask))
		{
			continue;
		}

		//local communication is ignored
		if ((ip_head->daddr & mask) == (ip & mask))
			continue;
		if (ip_head->saddr == ip)
			continue;

		thread_data = g_new(pcap_process_thread_param,1);

		thread_data->pcaphdr = *pcaphdr;

		thread_data->packet_contents = g_malloc(pcaphdr->len);

		memcpy((void*)(thread_data->packet_contents), packet_contents, pcaphdr->len);

		g_thread_pool_push(threadpool, thread_data, NULL);

	}
	//	int fno = pcap_fileno(pcap_handle);

	pcap_close(pcap_handle);
	return 0;
}
