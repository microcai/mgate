/*
 * clientmgr.c
 *
 *  Created on: 2010-4-8
 *      Author: cai
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

#include "i18n.h"
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
	g_object_class_install_property(gobjclass,CLIENT_NAME,
			g_param_spec_boolean("enable","enable","enable",TRUE,G_PARAM_CONSTRUCT|G_PARAM_READWRITE));
}

G_DEFINE_TYPE(Client,client,G_TYPE_OBJECT);

void client_init(Client * obj)
{
}

void client_finalize(GObject * gobj)
{
	Client * obj = (typeof(obj))gobj;
	g_free((gpointer)(obj->id));
	g_free((gpointer)(obj->name));
	G_OBJECT_CLASS(client_parent_class)->finalize(gobj);
}

static void client_set_property(GObject *object, guint property_id,const GValue *value, GParamSpec *pspec)
{
	Client * obj;

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

Client * client_new(const gchar * name, const gchar * id,const gchar * idtype)
{
	return g_object_new(G_TYPE_CLIENT,"name",name,"id",id,"idtype",idtype,NULL);
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

/**
 * clientmgr_init:
 *
 * 初始化 clientmgr, 分配管理结构。使用前一定要初始化。由 main() 调用
 */
void clientmgr_init()
{
	g_type_init();
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

	spin_read_write_rlock();

	g_tree_foreach(client_tree,findval_func,&ret);

	spin_read_write_runlock();

	return ret;
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
	spin_read_write_rlock();

	Client * ptr = (Client*)g_tree_lookup(client_tree,mac);

	spin_read_write_runlock();

	return ptr;
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
	spin_read_write_wlock();
	guchar * mac_ = g_strdup(mac);
	g_tree_insert(client_tree, mac_, client);
	spin_read_write_wunlock();
}

