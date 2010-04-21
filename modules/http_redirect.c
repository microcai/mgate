//
// C++ Interface: redirect_to_local_http
//
// Description:
//
//
// Author:  <king@tao.kingstone>, (C) 2009
//
// Copyright: See COPYING file that comes with this distribution
//
#include <stdio.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <time.h>
#include <pcap/pcap.h>
#include <libnet.h>

#include <glib.h>
#include <gio/gio.h>
#include <gmodule.h>

#include "global.h"
#include "pcap_hander.h"
#include "clientmgr.h"
#include "i18n.h"

#define	HTTP_PORT 20480

#define GROUP_NAME	"http_redirect"

static u_int8_t httphead[512];

static u_int8_t httphead_t[] =
"HTTP/1.0 302 Found\n"
"Location: %s\n"
"Connection:close\n\n"
"<html>\n\t<head>\n\t\t<meta http-equiv=\"Refresh\"content=\"0 ; "
"url=%s\">\n\t</head>\n</html>\n";

static in_addr_t redirector_ip;

void redirector_host_resove_by_dns(GObject *source_object, GAsyncResult *res,gpointer user_data)
{
	GList * hosts = g_resolver_lookup_by_name_finish(G_RESOLVER(source_object),res,NULL);

	if(hosts)
	{
		GList * it = g_list_first(hosts);

		do
		{
			GInetAddress * addr = (GInetAddress*)(it->data);

			if(g_inet_address_get_native_size(addr)==4)
			{
				memcpy(&redirector_ip,g_inet_address_to_bytes(addr),4);
				g_message(_("DNS result : %s"),g_inet_address_to_string(addr));
				break;
			}
		}while( it = g_list_next(it));
		g_resolver_free_addresses(hosts);
	}
	g_object_unref(source_object);
}

static void http_redirector_init(const gchar * desturl)
{
	char host[128];
	sscanf(desturl,"http://%128[^/]",host);
	redirector_ip = inet_addr(host);
	if (redirector_ip == INADDR_NONE)
	{
		g_debug(_("host in the url is not an ipv4 address, will do async dns lookup"));
		GResolver * dns =  g_resolver_get_default();
		g_resolver_lookup_by_name_async(dns,host,NULL,redirector_host_resove_by_dns,dns);
	}
	sprintf((char*) httphead, (char*) httphead_t, desturl, desturl);
}

static gboolean http_redirector( struct pcap_pkthdr * pkt, const guchar * content, gpointer user_data)
{
	/*******************************************************************
	 * here we use TCP
	 * when we recv a SYN=1,ACK=0 packet, we just send a syn=1,ack=1 packet
	 * that contains nothing
	 * then we push a packet taht contains
	 * 	HTTP/1.0 302 Found
	 * 	Location: http://192.168.0.1/
	 * 	connection:close
	 *
	 * 	please visit http://192.168.0.1
	 * 	and then we reset the connection
	 * ****************************************************************/
	struct tcphdr * tcp_head;
	Client * client;
	libnet_t * libnet = (libnet_t * ) user_data;
	if((client =  clientmgr_get_client_by_mac(content)) && client->enable )
	{
		//继续交给后续代码处理
		return FALSE ;
	}
	//非 enable 的客户端，现在要开始这般处理了,重定向到 ... 嘿嘿
	struct iphdr * ip_head = (typeof(ip_head))(content + LIBNET_ETH_H);

	if(ip_head->daddr == redirector_ip)
		return TRUE;

	g_debug(_("thread %p is doing the redirect stuff"),g_thread_self());

	//Retrive the tcp header
	tcp_head = (struct tcphdr*) ((char*) ip_head + ip_head->ihl * 4);

//	if (tcp_head->dest != HTTP_PORT)
//		return TRUE;

	u_int8_t tcp_flags = ((struct libnet_tcp_hdr *) tcp_head)->th_flags;

	if(tcp_flags == TH_SYN)
	{
		/********************************
		 * 对于这样的一个握手数据包
		 * 我们应该要建立连接了
		 * 回复一个syn ack 就是了
		 *********************************/
		// here we just echo ack and syn.
		libnet_build_tcp(ntohs(tcp_head->dest), ntohs(tcp_head->source), tcp_head->seq, ntohl(
				tcp_head->seq) + 1, TH_ACK | TH_SYN, 4096, 0, 0, 20, 0, 0,
				libnet, 0);

		libnet_build_ipv4(40, 0, 0, 0x4000, 63/*ttl*/, IPPROTO_TCP, 0,
				ip_head->daddr, ip_head->saddr, 0, 0, libnet, 0);

		libnet_build_ethernet(
				((struct libnet_ethernet_hdr *) content)->ether_shost,
				((struct libnet_ethernet_hdr *) content)->ether_dhost,
				ETHERTYPE_IP, 0, 0, libnet, 0);

		libnet_write(libnet);
		libnet_clear_packet(libnet);
	}else if (tcp_flags == (TH_PUSH | TH_ACK))
	{
		/*********************************************
		 *现在是发送页面的时候啦！
		 *********************************************/
		int SIZEHTTPHEAD = strlen((const char*) httphead);

		libnet_build_tcp(ntohs(tcp_head->dest), ntohs(tcp_head->source), ntohl(tcp_head->ack_seq),
				ntohl(tcp_head->seq) + ntohs(ip_head->tot_len) - 40, TH_ACK
						| TH_PUSH | TH_FIN, 4096, 0, 0, 20 + SIZEHTTPHEAD,
				httphead, SIZEHTTPHEAD, libnet, 0);

		libnet_build_ipv4(40 + SIZEHTTPHEAD, 0, 0, 0x4000, 63/*ttl*/,
				IPPROTO_TCP, 0, ip_head->daddr, ip_head->saddr, 0, 0, libnet, 0);

		libnet_build_ethernet(
				((struct libnet_ethernet_hdr*) content)->ether_shost,
				((struct libnet_ethernet_hdr*) content)->ether_dhost,
				ETHERTYPE_IP, 0, 0, libnet, 0);

		libnet_write(libnet);

		libnet_clear_packet(libnet);
	}
	else if (tcp_flags & TH_FIN)
	{
		/*********************************************************
		 *好，现在结束连接！
		 ********************************************************/
		libnet_build_tcp(ntohs(tcp_head->dest), ntohs(tcp_head->source), ntohl(tcp_head->ack_seq),
				ntohl(tcp_head->seq) + 1, TH_ACK, 4096, 0, 0, 20, 0, 0, libnet,
				0);
		libnet_build_ipv4(40, 0, 0, 0x4000, 63/*ttl*/, IPPROTO_TCP, 0,
				ip_head->daddr, ip_head->saddr, 0, 0, libnet, 0);

		libnet_build_ethernet(
				((struct libnet_ethernet_hdr*) content)->ether_shost,
				((struct libnet_ethernet_hdr*) content)->ether_dhost,
				ETHERTYPE_IP, 0, 0, libnet, 0);
		libnet_write(libnet);
		libnet_clear_packet(libnet);
	}
	return TRUE;
}

G_MODULE_EXPORT gchar * g_module_check_init(GModule *module)
{
	gboolean	enable;
	GError * err=NULL;
	gchar * url;

	char buf[LIBNET_ERRBUF_SIZE];

	enable = g_key_file_get_boolean(gkeyfile,GROUP_NAME,"enable",&err);

	if(err)
	{
		enable = TRUE;
		g_error_free(err);
		g_message("%s","[http_redirect]:[enable] not specified, default to enable");
		err = NULL;
	}

	if (!enable)
	{
		return "[http_redirect]:[enable=false], user disabled me";
	}

	url = g_key_file_get_string(gkeyfile, GROUP_NAME, "url", NULL);

	if(!url)
	{
		return "[http_redirect]:[url] not defined, please define one";
	}

	http_redirector_init(url);

	g_module_make_resident(module);

	libnet_t * libnet = libnet_init(LIBNET_RAW4, NULL, buf);
	pcap_hander_register_prepend(http_redirector, 0, IPPROTO_TCP, libnet);
	return NULL;
}
