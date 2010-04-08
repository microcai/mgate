/*
 * pcap_hander.c
 *
 *  Created on: 2010-4-8
 *      Author: cai
 */


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

#ifdef HAVE_GETTEXT
#include <locale.h>
#include <libintl.h>
#define _(x) gettext(x)
#define N_(x) (x)
#endif

typedef struct _pcap_hander{
	gpointer	FUNC;
	gpointer	user_data;
	struct _pcap_hander * next;
	struct _pcap_hander * prev;
	guint16		port;
	guint16		protocol;
}pcap_hander;

static pcap_hander pcap_hander_list = { .next = &pcap_hander_list , .prev = &pcap_hander_list} ;
GStaticMutex lock = G_STATIC_MUTEX_INIT;
volatile int read_count;
volatile gpointer * queue = NULL;
volatile int need_clean_up;
//延迟修改在这里发生
static void pcap_hander_acu_clean_up()
{
	if (g_atomic_int_compare_and_exchange(&need_clean_up,TRUE,FALSE))
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
	if( g_atomic_int_dec_and_test(&read_count) == 0)
		pcap_hander_acu_clean_up();
}

static inline void pcap_hander_rcu_write_lock()
{
	g_static_mutex_lock(&lock);
}

static inline void pcap_hander_rcu_write_unlock()
{
	g_static_mutex_unlock(&lock);
}

//hander use RCU so that we don't even need a lock
gpointer pcap_hander_register(gpointer FUNC,guint16 port,guint16 protocol,gpointer user_data)
{
	//构造一个 pcap_hander 结构
	pcap_hander * newhander = g_new0(pcap_hander,1);

	newhander->user_data = user_data;
	newhander->port = port;
	newhander->protocol = protocol;

	pcap_hander_rcu_write_lock();
	newhander->FUNC = FUNC ;
	newhander->next = NULL ;
	newhander->prev = & pcap_hander_list ;
	pcap_hander_list.next = newhander ;
	pcap_hander_rcu_write_unlock();
	return newhander;
}

void pcap_hander_ungister( gpointer hander)
{
	pcap_hander * hr = hander ;

	pcap_hander_rcu_write_lock();

	if( hr->next )
	{
		hr->next->prev = hr->prev;
	}
	if( hr->prev )
	{
		hr->prev->next = hr->next;
	}

	g_atomic_int_set(&need_clean_up,TRUE);

	g_atomic_pointer_compare_and_exchange(&queue,NULL,g_queue_new());
	g_queue_push_head(g_atomic_pointer_get(&queue),hr);
	pcap_hander_rcu_write_unlock();
}

GList * pcap_hander_get()
{

}


void pcap_hander_get_all()
{

}
