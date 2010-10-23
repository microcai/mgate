/*
 * http_server.c
 *
 *  Created on: 2010-4-20
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
#include "clientmgr.h"
#include "utils.h"
#include "http_server.h"
#include "i18n.h"
#include "global.h"
#include "htmlnode.h"
#include "smsapi.h"
#include "html_paths.h"

static void SoupServer_path_root(SoupServer *server, SoupMessage *msg,
		const char *path, GHashTable *query, SoupClientContext *client,
		gpointer user_data);

static SoupServer * server;

int start_server()
{
	GError * err = NULL;

	g_assert(gkeyfile);

	int port = g_key_file_get_integer(gkeyfile,"http","port",&err);
	if(err)
	{
		port = 8000;
		g_error_free(err);
		g_warning(_("using port %d as default duto the missing [http]:[port] settings"),port);
	}

	while (!(server = soup_server_new(SOUP_SERVER_ASYNC_CONTEXT,
			g_main_context_get_thread_default(), "port", port,
			SOUP_SERVER_SERVER_HEADER, PACKAGE_STRING " simple http server", NULL)))
	{
		g_warning(_("server failt to start at port %d, will use random port!"),port);
		port = 0;
	}

	g_debug(_("server started at port %u"),soup_server_get_port(server));

	soup_server_add_handler(server,"/login",SoupServer_path_login,NULL,NULL);

	soup_server_add_handler(server,"/info",SoupServer_path_info,NULL,NULL);

	soup_server_add_handler(server,"/getsmscode.asp",SoupServer_path_getsmscode,NULL,NULL);
	soup_server_add_handler(server,"/favicon.ico",SoupServer_path_static_file,NULL,NULL);
	soup_server_add_handler(server,"/",SoupServer_path_root,NULL,NULL);

	soup_server_add_handler(server,"/keep_alive",SoupServer_path_keep_alive,NULL,NULL);

	soup_server_run_async(server);

	sms_init();

	g_timeout_add_seconds(10,celect_usage,NULL);

	g_timeout_add_seconds(1,remove_outdated_phone_code_map,GINT_TO_POINTER(g_key_file_get_integer(gkeyfile,"sms","validtime",0)));

	return soup_server_get_port(server);
}

static void SoupServer_path_root(SoupServer *server, SoupMessage *msg,
		const char *path, GHashTable *query, SoupClientContext *client,
		gpointer user_data)
{
	if(g_strcmp0(path,"/")==0)
	{
		return SoupServer_path_static_file(server,msg,"/index.html",query,client,user_data);
	}

	return SoupServer_path_static_file(server,msg,path,query,client,user_data);
}

void soup_message_body_appender(const gchar * txt, SoupMessageBody * body)
{
	soup_message_body_append(body,SOUP_MEMORY_COPY,txt,strlen(txt));
}
