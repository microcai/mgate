//
// redirect_to_local_http
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

static void init_thread_libnet();
static void redirector_host_resove_by_dns(GObject *source_object, GAsyncResult *res,gpointer user_data);

#define	HTTP_PORT 0x5000
#define DNS_PORT  0x3500

#define GROUP_NAME	"http_redirect"

static u_int8_t httphead[512];

static u_int8_t httphead_t[] =
"HTTP/1.0 302 Found\n"
"Location: %s\n"
"Connection:close\n\n"
"<html>\n\t<head>\n\t\t<meta http-equiv=\"Refresh\"content=\"0 ; "
"url=%s\">\n\t</head>\n</html>\n";

static in_addr_t redirector_ip;
static GList*	 whiteip;
static __thread	 libnet_t * libnet = NULL;
static u_int8_t blank[128];

static void http_redirector_init(const gchar * desturl)
{
	char host[128];
	sscanf(desturl,"http://%128[^/^:]",host);
	redirector_ip = inet_addr(host);
	if (redirector_ip == INADDR_NONE)
	{
		g_debug(_("host in the url is not an ipv4 address, will do async dns lookup"));
		GResolver * dns =  g_resolver_get_default();
		g_resolver_lookup_by_name_async(dns,host,NULL,redirector_host_resove_by_dns,dns);
	}
	sprintf((char*) httphead, (char*) httphead_t, desturl, desturl);
}

static inline void	init_thread_libnet()
{
	if(!libnet)
	{
		static char buf[LIBNET_ERRBUF_SIZE];
		libnet = libnet_init(LIBNET_RAW4, NULL, buf);
	}
}

static gboolean http_redirector( pcap_process_thread_param * param, gpointer user_data,Kpolice * police)
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
	struct udphdr * udp_head;

	if(clientmgr_get_client_is_enable_by_mac(param->packet_linklayer_hdr+6))
	{
		//继续交给后续代码处理
		return FALSE ;
	}
	//非 enable 的客户端，现在要开始这般处理了,重定向到 ... 嘿嘿
	struct iphdr * ip_head = (typeof(ip_head))(param->packet_ip_contents);

	if (ip_head->daddr == redirector_ip)
	{
		return TRUE;
	}

	if (ip_head->saddr == redirector_ip)
	{
		return TRUE;
	}
	//如果是在 white list ....

	if (whiteip && g_list_find(whiteip, GINT_TO_POINTER(ip_head->daddr)))
		return FALSE;
	if (whiteip && g_list_find(whiteip, GINT_TO_POINTER(ip_head->saddr)))
		return FALSE;

	//	g_debug(_("thread %p is doing the redirect stuff"),g_thread_self());

	//Retrive the tcp header and udp header
	tcp_head = (struct tcphdr*) ((char*) ip_head + ip_head->ihl * 4);
	udp_head = (struct udphdr*) ((char*) ip_head + ip_head->ihl * 4);

#ifdef DEBUG_ONLY_HTTP_PORT
	if (tcp_head->dest != HTTP_PORT)
		return TRUE;
#endif

	//初始化libnet，每个线程一个 libnet ;)
	init_thread_libnet();

	// http 重定向
	if(ip_head->protocol == IPPROTO_TCP && tcp_head->dest == HTTP_PORT)
	{
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

			libnet_write(libnet);
			libnet_clear_packet(libnet);

			libnet_build_tcp(ntohs(tcp_head->dest), ntohs(tcp_head->source), tcp_head->seq, ntohl(
					tcp_head->seq) + 1, TH_ACK | TH_SYN, 4096, 0, 0, 20, 0, 0,
					libnet, 0);

			libnet_build_ipv4(40, 0, 0, 0x4000, 63/*ttl*/, IPPROTO_TCP, 0,
					ip_head->daddr, ip_head->saddr, 0, 0, libnet, 0);

		}else if (tcp_flags & TH_ACK)
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
		}
		else if (tcp_flags & TH_FIN)
		{
			/*********************************************************
			 *好，现在结束连接！
			 ********************************************************/
			libnet_build_tcp(ntohs(tcp_head->dest), ntohs(tcp_head->source), ntohl(tcp_head->ack_seq),
					ntohl(tcp_head->seq) + 1, TH_ACK|TH_RST, 4096, 0, 0, 20, 0, 0, libnet,
					0);
			libnet_build_ipv4(40, 0, 0, 0x4000, 63/*ttl*/, IPPROTO_TCP, 0,
					ip_head->daddr, ip_head->saddr, 0, 0, libnet, 0);

		}else{
			return FALSE;
		}
	}//其他 TCP 直接 RST
	else if( ip_head->protocol == IPPROTO_TCP)
	{
		libnet_build_tcp(ntohs(tcp_head->dest), ntohs(tcp_head->source), ntohl(tcp_head->ack_seq),
				ntohl(tcp_head->seq) + 1, TH_ACK|TH_RST, 4096, 0, 0, 20, 0, 0, libnet,
				0);
		libnet_build_ipv4(40, 0, 0, 0x4000, 63/*ttl*/, IPPROTO_TCP, 0,
				ip_head->daddr, ip_head->saddr, 0, 0, libnet, 0);

	}else if(ip_head->protocol == IPPROTO_UDP && udp_head->dest != DNS_PORT)
	{
		//现在是 UDP 的时代了
		libnet_build_udp(ntohs(udp_head->dest),ntohs(udp_head->source),
				sizeof(blank)+sizeof(struct udphdr),0,blank,sizeof(blank),libnet,0);
		libnet_build_ipv4(40, 0, 0, 0x4000, 63/*ttl*/, IPPROTO_UDP, 0,
				ip_head->daddr, ip_head->saddr, 0, 0, libnet, 0);

	}else
		return FALSE;

	if(param->linklayer_len == 14)
		libnet_build_ethernet(
			((struct libnet_ethernet_hdr *) param->packet_linklayer_hdr)->ether_shost,
			((struct libnet_ethernet_hdr *) param->packet_linklayer_hdr)->ether_dhost,
			ETHERTYPE_IP, 0, 0, libnet, 0);
	libnet_write(libnet);
	libnet_clear_packet(libnet);
	return TRUE;
}

G_MODULE_EXPORT gchar * g_module_check_init(GModule *module)
{
	gboolean	enable;
	GError * err=NULL;
	gchar * url;

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

	gchar *  ips = g_key_file_get_string(gkeyfile,GROUP_NAME,"whitelist",&err);


	if(err)
	{
		g_error_free(err);
	}else
	{
		GResolver * dns =  g_resolver_get_default();

		void resove_host_by_dns(GObject *source_object, GAsyncResult *res,gpointer user_data)
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
						in_addr_t ip;
						memcpy(&ip,g_inet_address_to_bytes(addr),4);
						whiteip = g_list_prepend(whiteip,GUINT_TO_POINTER(ip));
						g_message(_("%s's DNS result : %s"),(char*)user_data,g_inet_address_to_string(addr));
					}
				}while(( it = g_list_next(it)));
				g_resolver_free_addresses(hosts);
			}
			g_object_unref(source_object);
			g_free(user_data);
		}
		gchar *ptr;

		gchar * one_host =strtok_r(ips," \t",&ptr);


		while( one_host )
		{
			in_addr_t ip = inet_addr(one_host);
			if (ip == INADDR_NONE)
			{
				g_object_ref(dns);

				g_debug(_("host %s is not an ipv4 address, will do async dns lookup"),one_host);

				g_resolver_lookup_by_name_async(dns,one_host,NULL,resove_host_by_dns,g_strdup(one_host));
			}else
			{
				whiteip = g_list_prepend(whiteip,GUINT_TO_POINTER(ip));
			}
			one_host = strtok_r(NULL," \t",&ptr);
		}
		g_object_unref(dns);
	}



	g_module_make_resident(module);

	pcap_hander_register_prepend(http_redirector, 0, IPPROTO_TCP, libnet);
	return NULL;
}

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
		}while(( it = g_list_next(it)));
		g_resolver_free_addresses(hosts);
	}
	g_object_unref(source_object);
}
