/*
 * gsqlresult.c
 *
 *  Created on: 2010-4-14
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

#include "gsqlresult.h"

static void g_sql_result_class_init(GSQLResultClass * klass)
{
	GObjectClass	* gobjclass = G_OBJECT_CLASS(klass);

}

static void g_sql_result_init(GSQLResult * obj)
{
}


G_DEFINE_TYPE(GSQLResult,g_sql_result,G_TYPE_SQL_RESULT);
