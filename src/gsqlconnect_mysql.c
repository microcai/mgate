/*
 * gsqlconnect_mysql.c
 *
 *  Created on: 2010-4-13
 *      Author: cai
 */

/*
 * gsqlconnect.c
 *
 *  Created on: 2010-4-13
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
#include <glib-object.h>

#include "i18n.h"
#include "global.h"
#include "gsqlconnect.h"
#include "gsqlconnect_mysql.h"

enum G_SQL_CONNECT_MYSQL_PROPERTY{
	GSQL_CONNECT_MYSQL_HOST = 1,
	GSQL_CONNECT_MYSQL_USER,
	GSQL_CONNECT_MYSQL_PASSWD,
	GSQL_CONNECT_MYSQL_DB
};

static gboolean g_sql_connect_mysql_real_connect(GSQLConnect * obj);
static gboolean g_sql_connect_mysql_check_config(GSQLConnect * obj);
static void g_sql_connect_mysql_register_property(GSQLConnectMysqlClass * klass);
static void g_sql_connect_mysql_set_property(GObject *object,
		guint property_id, const GValue *value, GParamSpec *pspec);
static void g_sql_connect_mysql_get_property(GObject *object,
		guint property_id, GValue *value, GParamSpec *pspec);

static void g_sql_connect_mysql_class_init(GSQLConnectMysqlClass * klass)
{
	GObjectClass	* gobjclass = G_OBJECT_CLASS(klass);
	GSQLConnectClass * parent_class = (GSQLConnectClass*)klass;
	parent_class->connect = g_sql_connect_mysql_real_connect;
	parent_class->check_config = g_sql_connect_mysql_check_config;
	gobjclass->set_property = g_sql_connect_mysql_set_property;
	gobjclass->get_property = g_sql_connect_mysql_get_property;

	g_sql_connect_mysql_register_property(klass);
}

static void g_sql_connect_mysql_init(GSQLConnectMysql * obj)
{
	mysql_init(obj->mysql);
}


G_DEFINE_TYPE(GSQLConnectMysql,g_sql_connect_mysql,G_TYPE_SQL_CONNNECT);

gboolean g_sql_connect_mysql_real_connect(GSQLConnect * obj)
{
	g_assert(IS_G_SQL_CONNECT(obj));

	GSQLConnectMysql * mobj = (GSQLConnectMysql*)obj;

	gchar * host,*user,*passwd,*db;
	g_object_get(obj,"host",&host,"user",&user,"dbname",&db,NULL);

	if(mysql_real_connect(mobj->mysql,host,user,passwd,db,0,NULL,0))
		return TRUE;
	return FALSE;
//	mysql_real_connect(mobj->mysql,NULL,g_user,g_passwd,g_db,0,NULL,0);
}

#define INSTALL_PROPERTY_STRING(klass,id,name,defaultval,type) \
		g_object_class_install_property(klass,id, \
			g_param_spec_string(name,name,name,defaultval,type))

void g_sql_connect_mysql_register_property(GSQLConnectMysqlClass * klass)
{
	GObjectClass* objclass =(GObjectClass*) klass ;
	INSTALL_PROPERTY_STRING(objclass,GSQL_CONNECT_MYSQL_HOST,"host",NULL,G_PARAM_CONSTRUCT|G_PARAM_READWRITE);
	INSTALL_PROPERTY_STRING(objclass,GSQL_CONNECT_MYSQL_USER,"user","root",G_PARAM_CONSTRUCT|G_PARAM_READWRITE);
	INSTALL_PROPERTY_STRING(objclass,GSQL_CONNECT_MYSQL_PASSWD,"passwd","",G_PARAM_CONSTRUCT|G_PARAM_READWRITE);
	INSTALL_PROPERTY_STRING(objclass,GSQL_CONNECT_MYSQL_DB,"dbname","hotel",G_PARAM_CONSTRUCT|G_PARAM_READWRITE);

}

void g_sql_connect_mysql_set_property(GObject *object,guint property_id, const GValue *value, GParamSpec *pspec)
{
	GSQLConnectMysql * mobj = (GSQLConnectMysql*)object;

	g_return_if_fail(IS_G_SQL_CONNECT_MYSQL(mobj));

	switch (property_id) {
		case GSQL_CONNECT_MYSQL_HOST ... GSQL_CONNECT_MYSQL_DB:
		g_free(mobj->porperty_string[property_id]);
		mobj->porperty_string[property_id] = g_value_dup_string(value);
		break;
		default:
			break;
	}

}

void g_sql_connect_mysql_get_property(GObject *object,
		guint property_id, GValue *value, GParamSpec *pspec)
{
	GSQLConnectMysql * mobj = (GSQLConnectMysql*)object;

	g_return_if_fail(IS_G_SQL_CONNECT_MYSQL(mobj));

	switch (property_id)
	{
	case GSQL_CONNECT_MYSQL_HOST ... GSQL_CONNECT_MYSQL_DB:
		g_value_set_string(value, mobj->porperty_string[property_id]);
		break;
	default:
		break;
	}

}

gboolean g_sql_connect_mysql_check_config(GSQLConnect * obj)
{
	g_assert(IS_G_SQL_CONNECT(obj));

	gchar * g_db = g_key_file_get_string(gkeyfile,"mysql","db",NULL);
	gchar * g_user = g_key_file_get_string(gkeyfile,"mysql","user",NULL);
	gchar * g_passwd = g_key_file_get_string(gkeyfile,"mysql","passwd",NULL);

	if (g_db)
	{
		g_strchomp(g_strchug(g_user));
		if (strlen(g_db))
		{
			g_object_set(obj,"dbname",g_db,NULL);
		}
	}

	if(g_user)
	{
		g_strchomp(g_strchug(g_user));
		g_object_set(obj,"user",g_user,NULL);
	}
	if(g_passwd)
	{
		g_strchomp(g_strchug(g_passwd));
		g_object_set(obj,"passwd",g_passwd,NULL);
	}
	g_free(g_passwd);
	g_free(g_user);
	g_free(g_db);
	return TRUE;
}
