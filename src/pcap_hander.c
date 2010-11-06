/*
 * pcap_hander.c
 *
 *  Created on: 2010-4-8
 *
 *      Copyright 2010 薇菜工作室
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

#ifdef HAVE_CONFIG_H
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

#include "pcap_hander.h"

typedef struct _pcap_hander
{
	pcap_hander_callback FUNC;
	gpointer user_data;
	struct _pcap_hander * next;
	struct _pcap_hander * prev;
	guint16 port;
	guint16 protocol;
} pcap_hander;

static pcap_hander pcap_hander_list =
{ .next = &pcap_hander_list, .prev = &pcap_hander_list };
static GStaticMutex lock =
G_STATIC_MUTEX_INIT;
volatile int read_count;
volatile gpointer queue = NULL;
volatile int need_clean_up;
//延迟修改在这里发生
static inline void pcap_hander_rcu_clean_up()
{
	if (g_atomic_int_compare_and_exchange(&need_clean_up, TRUE, FALSE))
	{
		GQueue * q = (GQueue*) g_atomic_pointer_get(&queue);
		g_atomic_pointer_set(&queue,g_queue_new());
		g_queue_foreach(q, (GFunc) g_free, 0);
		g_queue_free(q);
	}
}

static inline void pcap_hander_rcu_read_lock()
{
	g_atomic_int_inc(&read_count);
}

//但没有人再使用的时候就可以进行延迟修改了
static inline void pcap_hander_rcu_read_unlock()
{
	if (g_atomic_int_dec_and_test(&read_count) == 0)
		pcap_hander_rcu_clean_up();
}

static inline void pcap_hander_rcu_write_lock()
{
	g_static_mutex_lock(&lock);
}

static inline void pcap_hander_rcu_write_unlock()
{
	g_static_mutex_unlock(&lock);
}

gpointer pcap_hander_register_prepend(pcap_hander_callback FUNC, guint16 port,guint16 protocol, gpointer user_data)
{
	//构造一个 pcap_hander 结构
	pcap_hander * newhander = g_new0(pcap_hander,1);

	newhander->user_data = user_data;
	newhander->port = port;
	newhander->protocol = protocol;
	newhander->FUNC = FUNC;
	pcap_hander_rcu_write_lock();

	newhander->prev = &pcap_hander_list;
	newhander->next = pcap_hander_list.next;
	pcap_hander_list.next->prev = newhander;
	pcap_hander_list.next = newhander;
	pcap_hander_rcu_write_unlock();
	return newhander;
}

gpointer pcap_hander_register(pcap_hander_callback FUNC, guint16 port,guint16 protocol, gpointer user_data)
{
	//构造一个 pcap_hander 结构
	pcap_hander * newhander = g_new0(pcap_hander,1);

	newhander->user_data = user_data;
	newhander->port = port;
	newhander->protocol = protocol;
	newhander->FUNC = FUNC;
	newhander->next = NULL;

	pcap_hander_rcu_write_lock();
	newhander->prev = pcap_hander_list.prev;
	pcap_hander_list.prev->next = newhander;
	pcap_hander_list.prev = newhander;
	pcap_hander_rcu_write_unlock();
	return newhander;
}

void pcap_hander_unregister(gpointer hander)
{
	pcap_hander * hr = hander;

	pcap_hander_rcu_write_lock();

	if (hr->next)
	{
		hr->next->prev = hr->prev;
	}
	if (hr->prev)
	{
		hr->prev->next = hr->next;
	}

	g_atomic_int_set(&need_clean_up,TRUE);

	g_atomic_pointer_compare_and_exchange(&queue, NULL, g_queue_new());
	g_queue_push_head(g_atomic_pointer_get(&queue), hr);
	pcap_hander_rcu_write_unlock();
}

//hander use RCU so that we don't even need a lock
//现在，读取还需要加锁么？
int pcap_hander_get(guint16 port, guint16 protocol,pcap_hander_callback_trunk out[])
{
	int count = 0;

	//这种锁够轻量级了吧!
	pcap_hander_rcu_read_lock();

	pcap_hander * hr = &pcap_hander_list;

	while ((hr = hr->next))
	{
		if (hr->port == 0 || hr->port == port)
		{
			if (hr->protocol == 0 || hr->protocol == protocol)
			{
				out[count].func = hr->FUNC;
				out[count].user_data = hr->user_data;
				count++;
			}
		}
	}
	pcap_hander_rcu_read_unlock();
	return count;
}

//hander use RCU so that we don't even need a lock
int pcap_hander_get_all(pcap_hander_callback_trunk out[])
{
	int count = 0;

	//这种锁够轻量级了吧!
	pcap_hander_rcu_read_lock();

	pcap_hander * hr = &pcap_hander_list;

	while ((hr = hr->next))
	{
		out[count].func = hr->FUNC;
		out[count].user_data = hr->user_data;
		count++;
	}
	pcap_hander_rcu_read_unlock();
	return count;
}
