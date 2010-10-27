/*
 * smsserver_connector.c
 *
 *  Created on: 2010-10-27
 *      Author: cai
 */


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <unistd.h>
#include <sys/resource.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <libsoup/soup.h>
#include "global.h"
#include "smsserver_connector.h"

typedef void (*smsserver_readycallback)
		(smsserver_result*, SoupMessage *msg,const char *path,GHashTable *query, SoupClientContext *client,gpointer user_data);

void smsserver_pinger_start()
{
	//获得 smshost
	//开始空白连接，哦哈哈
}

gboolean smsserver_is_online()
{
	return FALSE;
}

void smsserver_getcode( )
{

}


