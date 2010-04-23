/*
 * clientmgr.c
 *
 *  Created on: 2010-4-8
 *      Author: cai
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

#include "i18n.h"

#include "clientmgr.h"
#include "utils.h"

enum{
	CLIENT_NAME = 3, //名字
	CLIENT_ID, //身份证
	CLIENT_IP, // ip 地址
	CLIENT_MAC,
	CLIENT_ENABLE


};

static void client_set_property(GObject *object, guint property_id,const GValue *value, GParamSpec *pspec);
static void client_get_property(GObject *object, guint property_id,GValue *value, GParamSpec *pspec);

static void client_init(Client * obj);
static void client_finalize(GObject * obj);

static void client_class_init(ClientClass * klass)
{
	GObjectClass * gobjclass = G_OBJECT_CLASS(klass);
	gobjclass->set_property = client_set_property;
	gobjclass->get_property = client_get_property;
	gobjclass->finalize = client_finalize;

	g_object_class_install_property(gobjclass,CLIENT_NAME,
			g_param_spec_string("name","name","name","",G_PARAM_CONSTRUCT_ONLY|G_PARAM_READABLE));
	g_object_class_install_property(gobjclass,CLIENT_ID,
			g_param_spec_string("id","id","id","N/A",G_PARAM_CONSTRUCT_ONLY|G_PARAM_READABLE));
	g_object_class_install_property(gobjclass,CLIENT_IP,
			g_param_spec_int("ip","ip","ip",INADDR_ANY,INADDR_NONE,INADDR_NONE,G_PARAM_CONSTRUCT|G_PARAM_READWRITE));
	g_object_class_install_property(gobjclass,CLIENT_NAME,
			g_param_spec_boolean("enable","enable","enable",TRUE,G_PARAM_CONSTRUCT|G_PARAM_READWRITE));
}

G_DEFINE_TYPE(Client,client,G_TYPE_OBJECT);

void client_init(Client * obj)
{
	obj->name = g_string_new("");
	obj->id = g_string_new("");
}

void client_finalize(GObject * gobj)
{
	Client * obj = (typeof(obj))gobj;
	g_string_free(obj->id,TRUE);
	g_string_free(obj->name,TRUE);
	G_OBJECT_CLASS(client_parent_class)->finalize(gobj);
}

static void client_set_property(GObject *object, guint property_id,const GValue *value, GParamSpec *pspec)
{
	Client * obj;

	switch (property_id)
	{
	case CLIENT_NAME:
		obj->name = g_string_assign(obj->name,g_value_get_string(value));

		break;
	case CLIENT_ID:
		obj->id = g_string_assign(obj->id,g_value_get_string(value));
		break;
	case CLIENT_IP:
		obj->ip = g_value_get_int(value);
		break;
	case CLIENT_ENABLE:
		obj->enable = g_value_get_boolean(value);
		break;
	default:
		g_warn_if_reached();
		break;
	}

}

static void client_get_property(GObject *object, guint property_id,GValue *value, GParamSpec *pspec)
{
	Client * obj;
	switch (property_id)
	{
	case CLIENT_NAME:
		g_value_set_string(value,obj->name->str);
		break;
	case CLIENT_ID:
		g_value_set_string(value,obj->id->str);
		break;
	case CLIENT_IP:
		g_value_set_int(value,obj->ip);
		break;
	case CLIENT_ENABLE:
		g_value_set_boolean(value,obj->enable);
		break;
	default:
		g_warn_if_reached();
		break;
	}
}

Client * client_new(const gchar * name, const gchar * id)
{
	return g_object_new(G_TYPE_CLIENT,"name",name,"id",id,NULL);
}

//================================================================================================================
static volatile int	reader=0;
static volatile	int have_writer=0;

static GTree	* client_tree;
static gboolean g_tree_compare_func(gconstpointer a , gconstpointer b , gpointer user_data)
{
	return mac2uint64((guchar*)b) - mac2uint64((guchar*)a);
}

static inline void spin_read_write_rlock()
{
	while(have_writer)g_thread_yield();
	g_atomic_int_inc(&reader);
}

static inline void spin_read_write_runlock()
{
	g_atomic_int_add(&reader,-1);
}

static GStaticMutex lock= G_STATIC_MUTEX_INIT;

static inline void spin_read_write_wlock()
{
	g_atomic_int_inc(&have_writer);
	g_static_mutex_lock(&lock);
	while(g_atomic_int_get(&reader))
		g_thread_yield();
}

static inline void spin_read_write_wunlock()
{
	g_static_mutex_unlock(&lock);
	g_atomic_int_add(&have_writer,-1);
}


void clientmgr_init()
{
	g_type_init();
	client_tree = g_tree_new_full(g_tree_compare_func,0,g_free,g_object_unref);
}

static gboolean g_tree_travel_findval_func(gpointer key,gpointer val, gpointer user_data)
{
	Client * pval = * (Client **)user_data;
	in_addr_t ip = GPOINTER_TO_INT(pval);

	if(((Client*)val)->ip == ip)
	{
		*((Client **)user_data) = val ;
		return TRUE;
	}
	return FALSE;
}

Client * clientmgr_get_client_by_ip(in_addr_t ip)
{
	gpointer p_ip = GINT_TO_POINTER(ip);
	Client * ret = (Client*) p_ip;

	spin_read_write_rlock();

	g_tree_foreach(client_tree,g_tree_travel_findval_func,&ret);
	spin_read_write_runlock();

	if( ret == p_ip)
		return NULL;
	return ret;
}

Client * clientmgr_get_client_by_mac(const guchar * mac)
{
	spin_read_write_rlock();

	Client * ptr = (Client*)g_tree_lookup(client_tree,mac);

	spin_read_write_runlock();

	return ptr;
}

void clientmgr_insert_client_by_mac(guchar * mac, Client * client)
{
	spin_read_write_wlock();
	guchar * mac_ = g_strdup(mac);
	g_tree_insert(client_tree, mac_, client);
	spin_read_write_wunlock();
}

