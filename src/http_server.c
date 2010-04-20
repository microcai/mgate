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
#include <string.h>
#include <glib.h>
#include <libsoup/soup.h>
#include "http_server.h"
#include "i18n.h"
#include "global.h"

static void SoupServer_path_index(SoupServer *server, SoupMessage *msg,
		const char *path, GHashTable *query, SoupClientContext *client,
		gpointer user_data);
static void SoupServer_path_root(SoupServer *server, SoupMessage *msg,
		const char *path, GHashTable *query, SoupClientContext *client,
		gpointer user_data);

static SoupServer * server;

static gboolean finish(gpointer msg)
{
	g_debug(_("finish called"));
	soup_server_unpause_message(server,msg);
	return FALSE;
}

static gboolean idel(gpointer msg)
{
	g_timeout_add(1,finish,msg);
	return FALSE;
}

static void SoupServer_path_test(SoupServer *server, SoupMessage *msg,
		const char *path, GHashTable *query, SoupClientContext *client,
		gpointer user_data)
{
	static int i;

	soup_message_set_status(msg,SOUP_STATUS_OK);

	gchar * body = g_strdup_printf("<html><body>你好 %d</body></html>",i++);

	soup_message_set_response(msg,"text/html",SOUP_MEMORY_TAKE,body,strlen(body));
}


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

	soup_server_add_handler(server,"/test",SoupServer_path_test,NULL,NULL);

	soup_server_add_handler(server,"/",SoupServer_path_root,NULL,NULL);
	soup_server_add_handler(server,"/index.html",SoupServer_path_index,NULL,NULL);

	soup_server_run_async(server);

	return soup_server_get_port(server);
}


static void SoupServer_path_index(SoupServer *server, SoupMessage *msg,
		const char *path, GHashTable *query, SoupClientContext *client,
		gpointer user_data)
{
	static int i;

	soup_message_set_status(msg,SOUP_STATUS_OK);
	gchar * body = g_strdup_printf("<html><body>" PACKAGE_STRING "</body></html>",i++);

	soup_message_set_response(msg,"text/html",SOUP_MEMORY_TAKE,body,strlen(body));
}


static void SoupServer_path_root(SoupServer *server, SoupMessage *msg,
		const char *path, GHashTable *query, SoupClientContext *client,
		gpointer user_data)
{
	gchar * body;
	static int i;


	if(g_strcmp0(path,"/")==0)
	{
		return SoupServer_path_index(server,msg,path,query,client,user_data);
	}
	else
	{
		soup_message_set_status(msg,SOUP_STATUS_OK);
		body = g_strdup_printf("<html><body>你好 %d , 你访问的是 %s </body></html>", i++ ,path);
	}

	soup_message_set_response(msg,"text/html",SOUP_MEMORY_TAKE,body,strlen(body));
}
