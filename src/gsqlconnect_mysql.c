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

static gboolean g_sql_connect_mysql_real_connect(GSQLConnect * obj)
{
	GSQLConnectMysql * mobj = (GSQLConnectMysql*)obj;

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

	mysql_real_connect(mobj->mysql,NULL,g_user,g_passwd,g_db,NULL,NULL,NULL);

	g_free(g_passwd);
	g_free(g_user);
	g_free(g_db);
}

static void g_sql_connect_mysql_class_init(GSQLConnectMysqlClass * klass)
{
	GSQLConnectClass * parent_class = (GSQLConnectClass*)klass;
	parent_class->connect = g_sql_connect_mysql_real_connect;


}

static void g_sql_connect_mysql_init(GSQLConnectMysql * obj)
{
	mysql_init(obj->mysql);
}



G_DEFINE_TYPE(GSQLConnectMysql,g_sql_connect_mysql,G_TYPE_SQL_CONNNECT);

