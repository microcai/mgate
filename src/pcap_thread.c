/*
 *      pcap_thread.c - 抓包线程池
 *
 *      Copyright 2009-2010 薇菜工作室
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

#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <net/ethernet.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <pcap/pcap.h>
#include <pcap/sll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <glib.h>
#include <sys/resource.h>
#include <glib/gi18n.h>

#include "utils.h"
#include "pcap_thread.h"
#include "pcap_hander.h"
#include "clientmgr.h"
#include "global.h"
#include "traffic_status.h"

static void pcap_process_thread_func(gpointer _thread_data, Kpolice* police)
{

	struct in_addr ip;

	u_int16_t port;
	int i,j;
	pcap_process_thread_param * thread_data = _thread_data;

	ip.s_addr = thread_data->ip;

	const guchar * packet_content = thread_data->packet_ip_contents;

	struct iphdr * ip_head = (typeof(ip_head))thread_data->packet_ip_contents ; //(typeof(ip_head))(packet_content + thread_data->offset_skip_linklayer);

	/*********************************************************************
	 * for both UDP and TCP protocols ,source and destination port is
	 * just behind the IP header, and destination port is just behind
	 * the source port
	 *********************************************************************/
	port = *((u_int16_t*) (packet_content + ip_head->ihl * 4 + 2));

	pcap_hander_callback_trunk	handers[1024];

	//here we get a list of handler;
	bzero(handers, sizeof(handers));

	i = pcap_hander_get(port,ip_head->protocol,handers);

	//then we call these handler one by one
	for(j=0;j<i;j++)
	{
		if(handers[j].func(thread_data,handers[j].user_data,police))
			break;
	}
	g_free((void*)(thread_data->packet_linklayer_hdr));
	g_free(thread_data);
}

void *pcap_thread_func(void * thread_param)
{
	pcap_t * pcap_handle;
	struct ifreq rif;
	GError * err = NULL;
	bpf_u_int32 ip, mask;
	struct rlimit	limit;
	int		pcap_next_ex_ret;

	gsize offset_skip_linklayer = ETH_HLEN;

	char errbuf[PCAP_ERRBUF_SIZE];
	struct bpf_program bpf_filter =
	{ 0 };

	g_assert(gkeyfile);

	gchar * pcapfile = g_key_file_get_string(gkeyfile,"pcap","pcap",NULL);

	if(pcapfile)
	{
		pcap_handle = pcap_open_offline(pcapfile,errbuf);

		ip = 0;
		mask = 0;

		if(!pcap_handle)
		{
			g_warning("%s",errbuf);
			g_free(pcapfile);
			return 0;
		}

	}else
	{
		gchar * nic = g_key_file_get_string(gkeyfile,PACKAGE_NAME,"nic",NULL);
		if(nic)
			g_strchomp(g_strchug(nic));
		else
		{
			nic = g_strdup("eth0");
			g_warning(_("using %s as capturing interface"),nic);
		}

		memset(&rif,0,sizeof(rif));
		strcpy(rif.ifr_name,nic);

		pcap_handle = pcap_open_live(nic, 65536, 1, 0, errbuf);

		if (!pcap_handle)
		{
			g_warning( _("ERROR:can not open %s for capturing!\n"), nic);
			g_free(nic);
			return 0;
		}

		if (pcap_datalink(pcap_handle) != DLT_EN10MB )
		{
			g_warning(_("ERROR:%s is not an ethernet adapter\n"), nic);

			if (pcap_datalink(pcap_handle) == DLT_LINUX_SLL)
			{
				g_warning("Using Linux cooked sockets");
				offset_skip_linklayer = SLL_HDR_LEN;
			}else
			{
				g_free(nic);
				return 0;
			}
		}else
		{

		}

		pcap_lookupnet(nic, &ip, &mask, errbuf);

		g_free(nic);

		int sock = socket(AF_INET, SOCK_DGRAM, 0);

		if (!ioctl(sock, SIOCGIFADDR, &rif))
		{
			struct sockaddr_in * in_address;
			in_address = (struct sockaddr_in*)&(rif.ifr_addr);
			ip = in_address->sin_addr.s_addr;
		}

		gchar * filterstr = NULL;

		if (!ioctl(sock, SIOCGIFHWADDR, &rif))
		{
			gchar macstr[50];
			formatMAC((guchar*)(rif.ifr_hwaddr.sa_data),macstr);
			filterstr = g_strdup_printf("tcp or udp and ether src not %s",macstr);
		}

		close(sock);
		if(filterstr)
			pcap_compile(pcap_handle, &bpf_filter, filterstr, 1, 0);
		else
			pcap_compile(pcap_handle, &bpf_filter, "tcp or udp", 1, 0);
		g_free(filterstr);
		pcap_setfilter(pcap_handle, &bpf_filter);
		pcap_freecode(&bpf_filter);
	}

	struct pcap_pkthdr *pcaphdr;
	const u_char*packet_contents;

	getrlimit(RLIMIT_NOFILE,&limit);

	//解析出参数来
	gint num_threads = g_key_file_get_integer(gkeyfile,PACKAGE_NAME,"threads",&err);

	if(err)
	{
		num_threads = sysconf(_SC_NPROCESSORS_ONLN) * 2;
		g_message(_("[monitor]:[threads] not set, using 2x cpu (%d) threads"),num_threads);
		g_error_free(err);
		err = NULL;
	}
	if (num_threads <= 0)
		num_threads = 1;
	else if (num_threads > (limit.rlim_cur - 8) )
	{
		num_threads = (limit.rlim_cur - 8) & 0xFFFFFFC;

		g_message(_("Too many thread will exclude the resource limite, reduce to %d"),num_threads);

	}

	GThreadPool * threadpool = g_thread_pool_new((GFunc)pcap_process_thread_func, thread_param, num_threads, TRUE, NULL);

	traffic_status_init();

	pcap_process_thread_param * thread_data;

	for (;;)
	{
		if( (pcap_next_ex_ret=pcap_next_ex(pcap_handle, &pcaphdr, &packet_contents))<0)
		{
			if(pcap_next_ex_ret==-1)
				g_warn_if_reached();
			break;
		}
		if(pcap_next_ex_ret==0)
			continue;

		struct iphdr * ip_head = (typeof(ip_head))(packet_contents + offset_skip_linklayer);
		//	    //non TCP or UDP is ignored
		if (ip_head->protocol != IPPROTO_TCP && ip_head->protocol != IPPROTO_UDP)
			continue;

		//out -> in is ignored
		if ((ip_head->saddr & mask) != (ip & mask))
		{
			//流量统计需要从外到内的下行流量和上行流量
			traffic_packet_callback(ip,mask,ip_head);
			continue;
		}

#ifndef DEBUG
		//local communication is ignored
		if (ip_head->saddr == ip)
			continue;

		if ((ip_head->daddr & mask) == (ip & mask))
			continue;
#endif

//		ETH_HLEN

		thread_data = g_new(pcap_process_thread_param,1);

		thread_data->linklayer_len = offset_skip_linklayer;

		thread_data->pcaphdr = *pcaphdr;

		thread_data->packet_linklayer_hdr = g_memdup(packet_contents,pcaphdr->len);
		thread_data->packet_ip_contents = thread_data->packet_linklayer_hdr +  offset_skip_linklayer;

		thread_data->ip = ip;

		g_thread_pool_push(threadpool, thread_data, NULL);

		//处理了再统计，别干扰处理的响应时间
		traffic_packet_callback(ip,mask,ip_head);
	}
	pcap_close(pcap_handle);
	g_thread_pool_free(threadpool,FALSE,TRUE);

	if(pcapfile)
		exit(0);
	return 0;
}
