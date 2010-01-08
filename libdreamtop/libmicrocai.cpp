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

#include <sys/ioctl.h>
#include <sys/eventfd.h>
#include <sys/poll.h>
#include <net/if_arp.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>

#include <errno.h>

#include "libmicrocai.h"
#include "my_log.h"


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

static struct handler * head = 0, *tail = 0;
void* register_protocol_handler(PROTOCOL_HANDLER handler, int port, int IPPROTOCOL_TYPE)
{
    struct handler *p = new struct handler();
    p->magic = 'M';
    p->port = port;
    p->protocol_type = IPPROTOCOL_TYPE;
    p->handler = handler;
    if (tail) {
        tail->next = p;
        p->pre = tail;
        tail = p;
    } else {
        tail = head = p;
        head->pre = 0;
        tail->next = 0;
    }
    return (void *) p;
}

PROTOCOL_HANDLER* get_registerd_handler(int port, int IPPROTOCOL_TYPE)
{
    PROTOCOL_HANDLER*ret = 0;
    int i = 0;
    struct handler* p = head;
    ret = (PROTOCOL_HANDLER*)malloc(4096);
    memset(ret, 0, 4096);
    while (p)
    {
        if (p->magic != 'M')break;
        if ((p->port == port || p->port == 0) && p->protocol_type == IPPROTOCOL_TYPE)
            ret[i++] = p->handler;
        p = p->next;
    }
    if (i)return ret;
    free(ret);
    return 0;
}

int un_register_protocol_handler(void*p)
{
    if (p)
    {
        if (((struct handler *) p)->magic != 'M')
            return -1;

        if (((struct handler *) p)->pre)
            ((struct handler *) p)->pre->next = ((struct handler *) p)->next;
        else
            head = ((struct handler *) p)->next;
        if (((struct handler *) p)->next)
            ((struct handler *) p)->next->pre = ((struct handler *) p)->pre;
        else
            tail = ((struct handler *) p)->pre;
        delete (struct handler*)p;
        return 0;
    } else
        return -1;
}


void Wake_all(int __event)
{
	eventfd_t v=1;
	eventfd_write(__event,v);
	pthread_yield();
	eventfd_read(__event,&v);

}


int Wait_For_event(int __event,int timedout)
{
	pollfd f;
	f.fd = __event;
	f.events = POLLIN;
	f.revents = 0;

	int ret;

	for(;;)
	{
		ret = poll(&f,1,timedout);
		switch(ret)
		{
		case -1:
			if(errno != EINTR)
			return ret;
			else continue;
		case 0:
			return 1;
		case 1:
			if (f.revents & POLLIN)
				return 0;
			else
				return -1;
		}
	}
}

bool GetMac(const char *ip, char MAC_ADDR[],char mac_addr[])
{
	//向内核发送发起ARP查询
	int s = socket(PF_INET, SOCK_DGRAM, 0);
	struct arpreq arpr =
	{
	{ 0 } };

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
	log_printf(L_DEBUG_OUTPUT,"run: %s\n",strcmd);
//	pthread_atfork(0,0,0);
	system(strcmd.c_str());
}

static std::map<std::string,void*> registerd_val_map;
void * get_registered_val(std::string val)
{
	std::map<std::string,void*>::iterator it= registerd_val_map.find(val);
	return it->second;
}


