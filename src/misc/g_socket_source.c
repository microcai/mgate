/*
 * g_socket_source.c
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
#include <gio/gio.h>
#include <libsoup/soup.h>
#include "clientmgr.h"
#include "utils.h"
#include "i18n.h"
#include "global.h"
#include "g_socket_source.h"

guint g_socket_add_watch(GSocket *channel, GIOCondition condition,
		GSockIOFunc func, gpointer user_data)
{
	GSource * source = g_socket_create_source(channel,condition,0);
	return g_source_attach(source,NULL);
}
