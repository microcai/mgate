/*
 * http_server.c -- HTTP 服务器实现
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


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <unistd.h>
#include <sys/resource.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <libsoup/soup.h>
#include "utils.h"
#include "http_server.h"
#include "global.h"
#include "html_paths.h"
#include "smsserver_connector.h"
#include "mime.h"

static void SoupServer_path_root(SoupServer *server, SoupMessage *msg,
		const char *path, GHashTable *query, SoupClientContext *client,
		gpointer user_data);

static SoupServer * server;

int start_server()
{
	int port;
	GError * err = NULL;

	g_assert(gkeyfile);

	if(!start_by_systemd){

		port = g_key_file_get_integer(gkeyfile,"http","port",&err);

		if(err)
		{
			port = 8000;
			g_error_free(err);
			g_warning(_("using port %d as default due to the missing [http]:[port] settings"),port);
			err = NULL;
		}

		while (!(server = soup_server_new(SOUP_SERVER_ASYNC_CONTEXT,
				g_main_context_get_thread_default(), "port", port,
				SOUP_SERVER_SERVER_HEADER, PACKAGE_STRING " simple http server", NULL)))
		{
			g_warning(_("server failt to start at port %d, will use random port!"),port);
			port = 0;
		}

	}else
	{
//		soup_server_

//		soup_socket_new();

	}

	gchar *  httproot = g_key_file_get_string(gkeyfile,"http","root",&err);

	if(err)
	{
		g_error_free(err);
	}else
	{
		g_message(_("using %s as http root overlay"),httproot);
	}

	smsserver_pinger_start();

	g_debug(_("server started at port %u"),soup_server_get_port(server));

	soup_server_add_handler(server,"/login",SoupServer_path_login,NULL,NULL);

	soup_server_add_handler(server,"/info",SoupServer_path_info,NULL,NULL);

	soup_server_add_handler(server,"/log",SoupServer_path_log,NULL,NULL);

	soup_server_add_handler(server,"/favicon.ico",SoupServer_path_static_file,NULL,NULL);

	soup_server_add_handler(server,"/",SoupServer_path_root,httproot,NULL);

	soup_server_add_handler(server,"/keep_alive",SoupServer_path_keep_alive,NULL,NULL);

	soup_server_add_handler(server,"/getcode.html",SoupServer_path_getverifycode,NULL,NULL);

	soup_server_run_async(server);

	g_timeout_add_seconds(1,celect_usage,NULL);

	g_timeout_add_seconds(16,remove_outdated_inactive_client,GINT_TO_POINTER(30));

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
