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

static void g_sql_connect_class_init(GSQLConnectClass * klass)
{

}

static void g_sql_connect_init(GSQLConnect * klass)
{

}



G_DEFINE_TYPE(GSQLConnect,g_sql_connect,G_TYPE_OBJECT);

