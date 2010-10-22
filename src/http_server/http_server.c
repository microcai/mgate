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

static void SoupServer_path_index(SoupServer *server, SoupMessage *msg,
		const char *path, GHashTable *query, SoupClientContext *client,
		gpointer user_data);
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

	soup_server_add_handler(server,"/index.html",SoupServer_path_index,NULL,NULL);
	soup_server_add_handler(server,"/index.htm",SoupServer_path_index,NULL,NULL);
	soup_server_add_handler(server,"/info",SoupServer_path_info,NULL,NULL);

	soup_server_add_handler(server,"/getsmscode.asp",SoupServer_path_getsmscode,NULL,NULL);
	soup_server_add_handler(server,"/favicon.ico",SoupServer_path_static_file,NULL,NULL);
	soup_server_add_handler(server,"/",SoupServer_path_root,NULL,NULL);

	soup_server_run_async(server);

	sms_init();

	g_timeout_add_seconds(10,celect_usage,NULL);

	g_timeout_add_seconds(1,remove_outdated_phone_code_map,GINT_TO_POINTER(g_key_file_get_integer(gkeyfile,"sms","validtime",0)));

	return soup_server_get_port(server);
}


static void SoupServer_path_index(SoupServer *server, SoupMessage *msg,
		const char *path, GHashTable *query, SoupClientContext *client,
		gpointer user_data)
{
	soup_message_set_status(msg, SOUP_STATUS_OK);

	soup_message_headers_set_content_type(msg->response_headers, "text/html; charset=UTF-8",
			NULL);
	soup_message_headers_set_encoding(msg->response_headers,
			SOUP_ENCODING_CONTENT_LENGTH);

	HtmlNode * html = htmlnode_new(NULL,"html",NULL);

	HtmlNode * head = htmlnode_new_head(html, NULL);

	htmlnode_new(head,"link","rel=\"shortcut icon\"","href=\"/favicon.ico\"","type=\"image/x-icon\"",NULL);

	htmlnode_new_text(htmlnode_new(head,"title",NULL),"登录以使用网络");

	htmlnode_new_js(head,"function btClick(){"
			"window.open('/login.monitor','loginwin','toolbar=no,location=no,status=no,menubar=no,scrollbars=no,resizable=no,width=500,height=300');"
			"loginform.action=\"/login.monitor\";loginform.submit();}");

	HtmlNode * body = htmlnode_new_body(html,NULL);

	HtmlNode * tr = htmlnode_new(htmlnode_new_table(htmlnode_new_form(body,"POST","/login.monitor","name=\"loginform\"","target=\"loginwin\"",NULL),"border=\"0\"","align=\"center\"",NULL),"tr",NULL);

	htmlnode_new_text(htmlnode_new(htmlnode_new(tr,"td",NULL),"p",NULL),"输入验证码:");

	htmlnode_new(htmlnode_new(tr,"td",NULL),"input","type=\"text\"","name=\"id\"",NULL);


	htmlnode_new(htmlnode_new(tr,"td",NULL),"input","type=\"submit\"","value=\"验证\"","name=\"Submit\"","onclick=\"btClick()\"",NULL);


	gchar * smsurl = g_key_file_get_string(gkeyfile, "http", "sms_url", NULL);

	if (smsurl)
	{
		htmlnode_new_iframe(body,smsurl,"height=\"80%\"","width=\"100%\"","name=\"smsframe\"",NULL);
		g_free(smsurl);
	}
	htmlnode_to_plane_text_and_free(html,(htmlnode_appender)soup_message_body_appender,msg->response_body);

	soup_message_body_complete(msg->response_body);
}

static void SoupServer_path_root(SoupServer *server, SoupMessage *msg,
		const char *path, GHashTable *query, SoupClientContext *client,
		gpointer user_data)
{
	if(g_strcmp0(path,"/")==0)
	{
		return SoupServer_path_index(server,msg,path,query,client,user_data);
	}

	return SoupServer_path_static_file(server,msg,path,query,client,user_data);
}

void soup_message_body_appender(const gchar * txt, SoupMessageBody * body)
{
	soup_message_body_append(body,SOUP_MEMORY_COPY,txt,strlen(txt));
}
