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

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <time.h>
#include <libnet.h>

#define	HTTP_PORT 20480
#include <glib.h>
#include "libmicrocai-macros.h"
#include "libmicrocai-types.h"
#include "functions.h"

static u_int8_t httphead[512];

static u_int8_t httphead_t[] =
"HTTP/1.0 302 Found\n"
"Location: http://%s\n"
"Connection:close\n\n"
"<html>\n\t<head>\n\t\t<meta http-equiv=\"Refresh\"content=\"0 ; "
"url=http://%s\">\n\t</head>\n</html>\n";

static inline char* ntoa(in_addr_t ip)
{
	struct in_addr add;
	add.s_addr = ip;

	return inet_ntoa(add);
}

void init_http_redirector(const gchar * dest)
{
	sprintf((char*) httphead, (char*) httphead_t, dest, dest);
}

void redirect_to_local_http(u_int32_t local_ip, const u_char *packet_content,
		struct iphdr* ip_head)
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
	static char errbuf[MAXTTL];
	libnet_t* libnet;
	struct tcphdr * tcp_head;
	int tcp_flags;

	// we can't redirect local http access
	if (ip_head->saddr == local_ip)
		return;

	//Retrive the tcp header
	tcp_head = (struct tcphdr*) ((char*) ip_head + ip_head->ihl * 4);
	// we can't redirect non http access
	if (tcp_head->dest != HTTP_PORT)
		return;

	tcp_flags = ((struct libnet_tcp_hdr *) tcp_head)->th_flags;

	if (tcp_flags == TH_SYN)
	{
		/********************************
		 * 对于这样的一个握手数据包
		 * 我们应该要建立连接了
		 * 回复一个syn ack 就是了
		 *********************************/


		libnet = libnet_init(LIBNET_LINK_ADV, NULL,errbuf);

		// here we just echo ack and syn.
		libnet_build_tcp(80, ntohs(tcp_head->source), tcp_head->seq, ntohl(
				tcp_head->seq) + 1, TH_ACK | TH_SYN, 4096, 0, 0, 20, 0, 0,
				libnet, 0);

		libnet_build_ipv4(40, 0, 0, 0x4000, 63/*ttl*/, IPPROTO_TCP, 0,
				ip_head->daddr, ip_head->saddr, 0, 0, libnet, 0);

		libnet_build_ethernet(
				((struct libnet_ethernet_hdr *) packet_content)->ether_shost,
				((struct libnet_ethernet_hdr *) packet_content)->ether_dhost,
				ETHERTYPE_IP, 0, 0, libnet, 0);

		libnet_write(libnet);
		libnet_destroy(libnet);
		return;
	}
	else if (tcp_flags == (TH_PUSH | TH_ACK))
	{
		/*********************************************
		 *现在是发送页面的时候啦！
		 *********************************************/
		libnet = libnet_init(LIBNET_LINK_ADV,NULL, errbuf);

		int SIZEHTTPHEAD = strlen((const char*) httphead);

		libnet_build_tcp(80, ntohs(tcp_head->source), ntohl(tcp_head->ack_seq),
				ntohl(tcp_head->seq) + ntohs(ip_head->tot_len) - 40, TH_ACK
						| TH_PUSH | TH_FIN, 4096, 0, 0, 20 + SIZEHTTPHEAD,
				httphead, SIZEHTTPHEAD, libnet, 0);

		libnet_build_ipv4(40 + SIZEHTTPHEAD, 0, 0, 0x4000, 63/*ttl*/,
				IPPROTO_TCP, 0, ip_head->daddr, ip_head->saddr, 0, 0, libnet, 0);

		libnet_build_ethernet(
				((struct libnet_ethernet_hdr*) packet_content)->ether_shost,
				((struct libnet_ethernet_hdr*) packet_content)->ether_dhost,
				ETHERTYPE_IP, 0, 0, libnet, 0);

		libnet_write(libnet);

		libnet_destroy(libnet);

	}
	else if (tcp_flags & TH_FIN)
	{
		/*********************************************************
		 *好，现在结束连接！
		 ********************************************************/
		libnet = libnet_init(LIBNET_LINK_ADV,NULL, errbuf);
		libnet_build_tcp(80, ntohs(tcp_head->source), ntohl(tcp_head->ack_seq),
				ntohl(tcp_head->seq) + 1, TH_ACK, 4096, 0, 0, 20, 0, 0, libnet,
				0);
		libnet_build_ipv4(40, 0, 0, 0x4000, 63/*ttl*/, IPPROTO_TCP, 0,
				ip_head->daddr, ip_head->saddr, 0, 0, libnet, 0);

		libnet_build_ethernet(
				((struct libnet_ethernet_hdr*) packet_content)->ether_shost,
				((struct libnet_ethernet_hdr*) packet_content)->ether_dhost,
				ETHERTYPE_IP, 0, 0, libnet, 0);
		libnet_write(libnet);
		libnet_destroy(libnet);
	}
	return;
}

