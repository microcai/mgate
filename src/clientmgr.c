/*
 * clientmgr.c
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

/**
 * SECTION:clientmgr
 * @short_description: Client 管理
 * @title:ClientManager
 * @stability: Stable
 *
 * #Client 用来描述一个上网客户
 * clientmgr_* 系列函数用来管理系统上存在的上网客户
 */


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/syslog.h>
#include <sys/socket.h>
#include <net/ethernet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <string.h>
#include <glib.h>
#include <glib/gi18n.h>

#include "clientmgr.h"
#include "utils.h"

enum{
	CLIENT_NAME = 3, //名字
	CLIENT_ID, //身份证
	CLIENT_ID_TYPE , // 证件类型
	CLIENT_IP, // ip 地址
	CLIENT_IP_STR, // ip 地址, str类型
	CLIENT_MAC,
	CLIENT_ENABLE,
	CLIENT_ROOM
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

	/**
	 * Client:name:
	 *
	 * 客户的名字
	 */
	g_object_class_install_property(gobjclass,CLIENT_NAME,
			g_param_spec_string("name","name","name","",G_PARAM_CONSTRUCT_ONLY|G_PARAM_READWRITE));

	/**
	 * Client:id:
	 *
	 * 证件号码
	 */
	g_object_class_install_property(gobjclass,CLIENT_ID,
			g_param_spec_string("id","id","id","N/A",G_PARAM_CONSTRUCT_ONLY|G_PARAM_READWRITE));

	/**
	 * Client:idtype:
	 *
	 * 证件类型
	 */
	g_object_class_install_property(gobjclass,CLIENT_ID_TYPE,
			g_param_spec_string("idtype","idtype","idtype","096",G_PARAM_CONSTRUCT_ONLY|G_PARAM_READWRITE));
	/**
	 * Client:room:
	 *
	 * 客户住的房间号
	 */
	g_object_class_install_property(gobjclass,CLIENT_ROOM,
			g_param_spec_string("room","room","room","0000",G_PARAM_READWRITE));
	/**
	 * Client:ipstr:
	 *
	 * 字符串版本的当前客户使用的 ip
	 * 如果没有 ip 则为 NULL
	 */
	g_object_class_install_property(gobjclass,CLIENT_IP_STR,
			g_param_spec_string("ipstr","ipstr","ipstr","0.0.0.0",G_PARAM_READWRITE));
	/**
	 * Client:ip:
	 *
	 * in_addr_t 类型客户 ip
	 */
	g_object_class_install_property(gobjclass,CLIENT_IP,
			g_param_spec_uint("ip","ip","ip",INADDR_ANY,INADDR_NONE,INADDR_ANY,G_PARAM_CONSTRUCT|G_PARAM_READWRITE));

	/**
	 * Client:enable:
	 *
	 * 是否允许上网
	 */
	g_object_class_install_property(gobjclass,CLIENT_ENABLE,
			g_param_spec_boolean("enable","enable","enable",TRUE,G_PARAM_CONSTRUCT|G_PARAM_READWRITE));
}

G_DEFINE_TYPE(Client,client,G_TYPE_OBJECT);

void client_init(Client * obj)
{
	time(&obj->last_active_time);
}

void client_finalize(GObject * gobj)
{
	Client * obj = (typeof(obj))gobj;
	g_free((gpointer)(obj->id));
	g_free((gpointer)(obj->name));
	g_free((gpointer)(obj->room));
	g_free((gpointer)(obj->idtype));
	G_OBJECT_CLASS(client_parent_class)->finalize(gobj);
}

static void client_set_property(GObject *object, guint property_id,const GValue *value, GParamSpec *pspec)
{
	Client * obj = CLIENT(object);

	switch (property_id)
	{
	case CLIENT_NAME:
		obj->name = g_value_dup_string(value);
		break;
	case CLIENT_ID:
		obj->id = g_value_dup_string(value);
		break;
	case CLIENT_ID_TYPE:
		obj->idtype = g_value_dup_string(value);
		break;
	case CLIENT_IP_STR:
		obj->ip = inet_addr(g_value_get_string(value));
		break;
	case CLIENT_ROOM:
		obj->room = g_value_dup_string(value);
		break;
	case CLIENT_IP:
		obj->ip = g_value_get_uint(value);
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
	Client * obj = CLIENT(object);
	gchar * ip_str;
	switch (property_id)
	{
	case CLIENT_NAME:
		g_value_set_string(value,obj->name);
		break;
	case CLIENT_ID:
		g_value_set_string(value,obj->id);
		break;
	case CLIENT_ID_TYPE:
		g_value_set_string(value,obj->idtype);
		break;
	case CLIENT_IP_STR:
		ip_str = g_strdup_printf("%03d.%03d.%03d.%03d",
				((guchar*)&(obj->ip))[0],
				((guchar*)&(obj->ip))[1],
				((guchar*)&(obj->ip))[2],
				((guchar*)&(obj->ip))[3]);
		g_value_take_string(value,ip_str);
		break;
	case CLIENT_ROOM:
		g_value_set_string(value,obj->room);
		break;
	case CLIENT_IP:
		g_value_set_uint(value,obj->ip);
		break;
	case CLIENT_ENABLE:
		g_value_set_boolean(value,obj->enable);
		break;
	default:
		g_warn_if_reached();
		break;
	}
}

Client * client_new(const gchar * name, const gchar * id,const gchar * idtype,guchar mac[6])
{
	Client * client= g_object_new(G_TYPE_CLIENT,"name",name,"id",id,"idtype",idtype,"room","",NULL);
	memcpy(client->mac,mac,6);
	return client;
}

static volatile GTree	* client_tree;
static gboolean g_tree_compare_func(gconstpointer a , gconstpointer b , gpointer user_data)
{
	return mac2uint64((guchar*)b) - mac2uint64((guchar*)a);
}

static GStaticRWLock lock= G_STATIC_RW_LOCK_INIT;

/**
 * clientmgr_init:
 *
 * 初始化 clientmgr, 分配管理结构。使用前一定要初始化。由 main() 调用
 */
void clientmgr_init()
{
	client_tree = g_tree_new_full(g_tree_compare_func,0,g_free,g_object_unref);
}

/**
 * clientmgr_get_client_by_ip:
 * @ip : 客户的ip地址
 * Returns: #Client , NULL 没有找到
 * 通过给定的 @ip 地址寻找拥有此@ip地址的客户
 */
Client * clientmgr_get_client_by_ip(in_addr_t ip)
{
	Client * ret = NULL;

	gboolean findval_func(gpointer key,gpointer val, gpointer user_data)
	{
		if(((Client*)val)->ip == ip)
		{
			ret = val ;
			return TRUE;
		}
		return FALSE;
	}

	g_static_rw_lock_writer_lock(&lock);

	g_tree_foreach(client_tree,findval_func,&ret);

	g_static_rw_lock_writer_unlock(&lock);

	return ret;
}

static Client * clientmgr_get_client_by_mac_internal(const guchar * mac)
{
	return (Client*)g_tree_lookup(client_tree,mac);
}
/**
 * clientmgr_get_client_by_mac:
 * @mac : mac 地址，6 个字节
 * Returns: #Client, NULL not found
 *
 * 通过 @mac 地址寻找特定的客户
 */
Client * clientmgr_get_client_by_mac(const guchar * mac)
{
	g_static_rw_lock_reader_lock(&lock);
	Client * ptr = clientmgr_get_client_by_mac_internal(mac);
	g_static_rw_lock_reader_unlock(&lock);
	return ptr;
}

gboolean clientmgr_get_client_is_enable_by_mac(const guchar * mac)
{
	gboolean enable = FALSE;
	g_static_rw_lock_reader_lock(&lock);
	Client * ptr = clientmgr_get_client_by_mac_internal(mac);
	if(ptr)
	{
		enable = ptr->enable ;
	}
	g_static_rw_lock_reader_unlock(&lock);
	return enable;
}

static gboolean clientmgr_reomve_client_internal(Client * client)
{
	return g_tree_remove(client_tree,client->mac);
}

gboolean clientmgr_reomve_client(Client * client)
{
	gboolean ret;

	g_static_rw_lock_writer_lock(&lock);
	ret = clientmgr_reomve_client_internal(client);
	g_static_rw_lock_writer_unlock(&lock);

	return ret;
}

void clientmgr_reomve_outdate_client(gulong	inactive_time_allowed, void (*removethis)(Client * client) )
{
	time_t cutime;
	time(&cutime);

	gboolean remove_outdate(gpointer key, Client * client, GTree * newtree)
	{
		if( client->remove_outdate && ( (cutime - client->last_active_time) > inactive_time_allowed))
		{
			removethis(client);
		}else
		{
			//add the unit to new tree
			g_tree_insert(newtree,g_memdup(key,6),g_object_ref(client));
		}
		return FALSE;
	}

	g_static_rw_lock_writer_lock(&lock);

	GTree * oldclient_tree = client_tree;

	clientmgr_init();

	g_tree_foreach(oldclient_tree,(GTraverseFunc)remove_outdate,client_tree);

	g_tree_unref(oldclient_tree);

	g_static_rw_lock_writer_unlock(&lock);
}

/**
 * clientmgr_insert_client_by_mac:
 * @mac : mac 地址
 * @client : #Client 客户
 *
 * 加入一个客户到管理列队。此后将能通过 clientmgr_get_client_by_ip()
 * clientmgr_get_client_by_mac() 找到他
 */
void clientmgr_insert_client_by_mac(guchar * mac, Client * client)
{
	g_static_rw_lock_writer_lock(&lock);

	Client* old = clientmgr_get_client_by_mac_internal(mac);

	if(old)
		clientmgr_reomve_client_internal(old);

	guchar * mac_ = g_memdup(mac,6);
	g_tree_insert(client_tree, mac_, client);

	g_static_rw_lock_writer_unlock(&lock);
}

