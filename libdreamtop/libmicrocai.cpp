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
static pthread_cond_t wait_cond=PTHREAD_COND_INITIALIZER;
void Wake_all()
{
	pthread_cond_broadcast( &wait_cond);
}


int Wait_For_event(int timedout)
{
	struct timespec tsm;
	pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
	tsm.tv_nsec = timedout %1000;
	tsm.tv_sec = timedout / 1000;

	pthread_mutex_lock(&m);
	int ret = pthread_cond_timedwait(&wait_cond,&m, &tsm);
	pthread_mutex_unlock(&m);
	return ret;
}

bool GetMac(const char *ip, char MAC_ADDR[],char mac_addr[])
{
	//向内核发送发起ARP查询
	int s = socket(PF_INET, SOCK_DGRAM, 0);
	struct arpreq arpr ={{0}};

	arpr.arp_flags = ATF_MAGIC;
	arpr.arp_pa.sa_family = AF_INET;
	((struct sockaddr_in*) (&(arpr.arp_pa)))->sin_addr.s_addr = inet_addr(ip);

	//遍历所有以太网卡

	int n = 0;
	do
	{
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


void run_cmd(const std::string & strcmd )
{
	//在这里我不得不考虑system失败会导致的资源泄漏。
//	pthread_mutex_lock()
	log_printf(L_DEBUG_OUTPUT,"run: %s\n",strcmd.c_str());
//	pthread_atfork(0,0,0);
	system(strcmd.c_str());
}

static std::map<std::string,void*> registerd_val_map;
void * get_registered_val(std::string val)
{
	std::map<std::string,void*>::iterator it= registerd_val_map.find(val);
	return it->second;
}


