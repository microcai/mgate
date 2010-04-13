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
#include "gsqlconnect.h"
#include "gsqlconnect_mysql.h"

static gboolean g_sql_connect_mysql_real_connect(GSQLConnect * obj)
{
	gchar * host = NULL;
	g_object_get(obj,"host",&host,NULL);

}

static void g_sql_connect_mysql_class_init(GSQLConnectMysqlClass * klass)
{

}

static void g_sql_connect_mysql_init(GSQLConnectMysql * klass)
{

}



G_DEFINE_TYPE(GSQLConnectMysql,g_sql_connect_mysql,G_TYPE_SQL_CONNNECT);

