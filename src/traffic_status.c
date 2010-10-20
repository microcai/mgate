/*
 * traffic_status.c - 流量监控和显示 -- 基于ip地址
 *
 *  Created on: 2010-10-16
 *      Author: cai
 */

#include <stdio.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <time.h>

#include <glib.h>
#include <gio/gio.h>

#include "global.h"
#include "clientmgr.h"
#include "i18n.h"
#include "traffic_status.h"

enum directions{
	UP =1 ,
	DOWN =2
};

typedef struct ip_traffice_thread_param{
	in_addr_t	ip;
	gsize	lengh;
	enum directions dir;
}ip_traffice_thread_param;

static void ip_traffice_thread_param_free(ip_traffice_thread_param*data)
{
	g_slice_free(ip_traffice_thread_param,data);
}


struct current_updown{
	volatile guint	  up;
	volatile guint	  down;
};

typedef struct ip_traffic_status
{
	in_addr_t ip;
	guint64	  up;
	guint64	  down;
	time_t	  last_active;
	//当前一秒的上传下载 aka 速率
	struct current_updown	currents[8];
	unsigned current:3;
}ip_traffic_status;

static GTree	*ipstatus;
static GAsyncQueue * queue;
static GStaticMutex	lock = G_STATIC_MUTEX_INIT;

static gboolean mark_eclipsed(gpointer  key,struct ip_traffic_status * status)
{
	status->current ++;//= !status->current;
	status->currents[status->current].up = status->currents[status->current].down = 0;
	return FALSE;
}

static gpointer update_ip_trafffic(gpointer  queue)
{
	ip_traffice_thread_param * data;

	GTimeVal endtime[1];

	g_get_current_time(endtime);
	g_time_val_add(endtime,1000000);

	for(;;)
	{
		data = g_async_queue_timed_pop(queue,endtime);

		if((gpointer)data == (gpointer)(-1))
		{
			g_async_queue_unref(queue);
			break;
		}

		g_static_mutex_lock(&lock);

		if(data)
		{
			gsize size = data->lengh;

			struct ip_traffic_status * status;
			status  = g_tree_lookup(ipstatus,GUINT_TO_POINTER(data->ip));
			if(!status)
			{
				g_debug("new ip %d",data->ip);
				status = g_slice_new0(struct ip_traffic_status);
				status->ip = data->ip;
				g_tree_insert(ipstatus,GUINT_TO_POINTER(data->ip),status);
			}

			if(data->dir == UP)
			{
				status->up += size;
				status->currents[status->current].up += size;
			}
			if (data->dir == DOWN)
			{
				status->down += size;
				status->currents[status->current].down += size;
			}
			time(&status->last_active);
			ip_traffice_thread_param_free(data);

		}else //过一秒了，可以那个了
		{
			g_time_val_add(endtime,1000000);
			g_tree_foreach(ipstatus,(GTraverseFunc)mark_eclipsed,0);
		}
		g_static_mutex_unlock(&lock);
	}
	return NULL;
}

static gboolean compare_ip(gconstpointer a , gconstpointer b)
{
	return GPOINTER_TO_UINT(a) - GPOINTER_TO_UINT(b);
}

static void free_value(ip_traffic_status * status)
{
	g_slice_free(ip_traffic_status,status);
}

void traffic_status_init()
{
	ipstatus = g_tree_new_full((GCompareDataFunc)compare_ip,NULL,NULL,(GDestroyNotify)free_value);
	queue = g_async_queue_new_full((GDestroyNotify)ip_traffice_thread_param_free);
	g_thread_create(update_ip_trafffic,queue,0,0);
}

void traffic_status_end()
{
	g_warn_if_reached();
	//TODO:	释放结构
	ip_traffic_reset_all();
}

static void update_ip_trafffic_info(in_addr_t ip ,enum directions direction, gsize size)
{
	ip_traffice_thread_param * data = g_slice_new0(ip_traffice_thread_param);
	data->ip = ip;
	data->dir = direction;
	data->lengh = size;

	g_async_queue_push(queue,data);
}

static __inline gboolean valid_ip(in_addr_t _ip)
{
	register int i;
	register guchar * ip = (guchar*)&_ip;

	for(i=0;i<4;i++)
	{
		if(G_UNLIKELY(ip[i]==0 || ip[i]==0xff))
			return FALSE;
	}
	return TRUE;
}


void traffic_packet_callback ( in_addr_t ip, in_addr_t mask , struct iphdr * ip_head)
{
	gsize	content_lengh;
	//获得 ip 头部, tcp 头部，udp 头部
	struct tcphdr* tcp_head = (typeof(tcp_head))(( (char*)ip_head) + ip_head->ihl*4);
	struct udphdr* ucp_head = (typeof(ucp_head))(( (char*)ip_head) + ip_head->ihl*4);

	if(ip_head->protocol == IPPROTO_TCP)
	{
		//按照每个 TCP 的数据内容来算流量
		content_lengh = (ip_head->tot_len - ip_head->ihl - tcp_head->doff);

	}else if(ip_head->protocol == IPPROTO_UDP)
	{
		//按照每个 UDP 的数据内容来算流量
		content_lengh = (ip_head->tot_len - ip_head->ihl - sizeof(struct udphdr));
	}

	//加入某个 ip 的流量中的上行还是下行呢？

	//这个是属于上行
	if ((ip_head->saddr & mask) == (ip & mask) && valid_ip(ip_head->saddr) )
	{
		update_ip_trafffic_info(ip_head->saddr, UP, content_lengh);
	}

	//这个属于下行
	if((ip_head->daddr & mask) == (ip & mask) && valid_ip(ip_head->daddr) )
	{
		update_ip_trafffic_info(ip_head->daddr, DOWN ,content_lengh);
	}
}

void ip_traffic_reset_ip(in_addr_t ip)
{
	g_static_mutex_lock(&lock);

	g_tree_remove(ipstatus,GUINT_TO_POINTER(ip));

	g_static_mutex_unlock(&lock);
}

void ip_traffic_reset_all()
{
	g_static_mutex_lock(&lock);
	g_tree_ref(ipstatus);
	g_tree_destroy(ipstatus);
	g_static_mutex_unlock(&lock);
}

IPStatus * ip_traffic_get_status(gsize * numberofips)
{
	IPStatus * ret = NULL;

	g_static_mutex_lock(&lock);
	*numberofips = g_tree_nnodes(ipstatus);
	if(*numberofips > 0)
	{
		ret = g_new0(IPStatus,*numberofips);

		int i=0;

		gboolean clectstatus(gpointer key,struct ip_traffic_status * status,IPStatus * ret)
		{
			ret[i].ip = status->ip;
			ret[i].up = status->up;
			ret[i].down = status->down;
			for( int j=0;j < 8 ;j++ )
			{
				if (j != status->current)
				{
					ret[i].downspeed += status->currents[j].down;
					ret[i].upspeed += status->currents[j].up;
				}
			}
			ret[i].downspeed/=7;
			ret[i].upspeed/=7;
			ret[i].last_active = status->last_active;
			i++;
			return FALSE;
		}
		g_tree_foreach(ipstatus,(GTraverseFunc)clectstatus,ret);
	}
	g_static_mutex_unlock(&lock);
	return ret;
}
