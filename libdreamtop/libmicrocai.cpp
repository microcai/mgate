/*
 * libmicrocai.c
 *
 * Copyright 2009 microcai <microcai@sina.com>
 *
 * See COPYING for more details about this software's license
 */

#include <iostream>
#include <map>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>

#include <sys/ioctl.h>
#include <net/if_arp.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <iconv.h>
#include <errno.h>

#include "libmicrocai.h"

u_int16_t checksum(u_int16_t *buffer, int size)
{
    uint32_t cksum = 0;

    while (size > 1)
    {
        cksum += *buffer++;
        size -= sizeof ( unsigned short);
    }

    if (size)
        cksum += *buffer;

    cksum = (cksum >> 16) + (cksum & 0xffff);

    cksum += (cksum >> 16);

    return (uint16_t) (~cksum);
}

bool GetMac(const char *ip, char MAC_ADDR[],char mac_addr[])
{
	//向内核发送发起ARP查询
	int s = socket(PF_INET, SOCK_DGRAM, 0);

	//遍历所有以太网卡

	int n = 0;
	do
	{
		struct arpreq arpr ={{0}};

		arpr.arp_flags = ATF_MAGIC;
		arpr.arp_pa.sa_family = AF_INET;
		((struct sockaddr_in*) (&(arpr.arp_pa)))->sin_addr.s_addr = inet_addr(ip);

		sprintf(arpr.arp_dev, "eth%d", n);
		n++;
		//通过ioctl获得arp
		int ret = ioctl(s, SIOCGARP, &arpr);
		if (ret == 0)
		{
			close(s);
			//获得MAC地址;
			u_char* d = (u_char*) arpr.arp_ha.sa_data;
			if(mac_addr)
				memcpy(mac_addr,d,6);
			if(MAC_ADDR)
				sprintf(MAC_ADDR, "%02x:%02x:%02x:%02x:%02x:%02x", d[0], d[1], d[2], d[3], d[4], d[5]);
			return true;
		}
		if (errno == ENXIO)
			continue;
		close (s);
		return false;
	} while(errno != ENODEV);
	close(s);
	return false;
}


void run_cmd(const CString & strcmd )
{
	//在这里我不得不考虑system失败会导致的资源泄漏。
//	pthread_mutex_lock()
	log_printf(L_DEBUG_OUTPUT,"run: %s\n",strcmd.c_str());
//	pthread_atfork(0,0,0);
	system(strcmd);
}

int utf8_gbk(char *outbuf, size_t outlen, const char *inbuf, size_t inlen)
{
	iconv_t cd;
	char **pin = (char**)&inbuf;
	char **pout = &outbuf;

	cd = iconv_open( "GBK","UTF-8");
	if (cd == 0)
		return -1;

	memset(outbuf, '\0', outlen);
	if (iconv(cd, pin, &inlen, pout, &outlen) == (size_t) -1)
	{
		return -1;
	}
	iconv_close(cd);

	return 0;
}

void nat_disable_ip(const char *  ip)
{
	CString cmd;
	cmd.Format("iptables -D FORWARD -s %s -j ACCEPT", ip);
	run_cmd(cmd);
}

void nat_enbale_ip(const char * ip)
{
	CString cmd;
	cmd.Format("iptables -A FORWARD -s %s -j ACCEPT", ip);
	run_cmd(cmd);
}
