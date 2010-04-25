/*
 * libmicrocai.c
 *
 * Copyright 2009 microcai <microcai@sina.com>
 *
 * See COPYING for more details about this software's license
 */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <syslog.h>
#include <sys/ioctl.h>
#include <net/if_arp.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <iconv.h>
#include <errno.h>
#include <string.h>
#include <glib.h>


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



#ifdef ENABLE_HOTEL

void run_cmd(const gchar *  strcmd )
{
	//在这里我不得不考虑system失败会导致的资源泄漏。
	syslog(LOG_NOTICE,"run: %s\n",strcmd);
	int ret = system(strcmd);
}

gboolean GetMac(const char *ip, char MAC_ADDR[],u_char mac_addr[])
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
				formatMAC(d,MAC_ADDR);
			return TRUE;
		}
		if (errno == ENXIO)
			continue;
		close (s);
		return FALSE;
	} while(errno != ENODEV);
	close(s);
	return FALSE;
}

#endif
