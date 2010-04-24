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
#include <stdio.h>
#include <string.h>
#include <glib.h>
#include <libsoup/soup.h>
#include "http_server.h"
#include "i18n.h"
#include "global.h"
#include "monitor_icon.h"
#include "htmlnode.h"

static void SoupServer_path_info(SoupServer *server, SoupMessage *msg,
		const char *path, GHashTable *query, SoupClientContext *client,
		gpointer user_data);
static void SoupServer_path_index(SoupServer *server, SoupMessage *msg,
		const char *path, GHashTable *query, SoupClientContext *client,
		gpointer user_data);
static void SoupServer_path_root(SoupServer *server, SoupMessage *msg,
		const char *path, GHashTable *query, SoupClientContext *client,
		gpointer user_data);
static void SoupServer_path_login(SoupServer *server, SoupMessage *msg,
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
	soup_server_add_handler(server,"/login",SoupServer_path_login,NULL,NULL);

	soup_server_add_handler(server,"/",SoupServer_path_root,NULL,NULL);
	soup_server_add_handler(server,"/index.html",SoupServer_path_index,NULL,NULL);
	soup_server_add_handler(server,"/index.htm",SoupServer_path_index,NULL,NULL);
	soup_server_add_handler(server,"/info",SoupServer_path_info,NULL,NULL);

	void SoupServer_path_root_icon(SoupServer *server, SoupMessage *msg,
			const char *path, GHashTable *query, SoupClientContext *client,
			gpointer user_data)
	{
		soup_message_set_status(msg, SOUP_STATUS_OK);
		soup_message_set_response(msg,"image/x-icon",SOUP_MEMORY_STATIC,
				monitor_icon,sizeof(monitor_icon));
	}

	soup_server_add_handler(server,"/favicon.ico",SoupServer_path_root_icon,NULL,NULL);

	soup_server_run_async(server);

	return soup_server_get_port(server);
}

const gchar * html_begin = "<html>\n";
const gchar * html_head_begin = "\t<head>\n";
const gchar * html_head_context = "";
const gchar * html_head_close = "\n\t</head>\n";

const gchar * html_body_begin = "\t<body>\n";

const gchar * html_body_close = "\n\t</body>";
const gchar * html_close = "\n</html>";

static void monitor_http_append_html_head(SoupMessageBody * body,const char * title)
{
	soup_message_body_append(body,SOUP_MEMORY_STATIC,html_begin,strlen(html_begin));
	soup_message_body_append(body,SOUP_MEMORY_STATIC,html_head_begin,strlen(html_head_begin));

	gchar * title_ = g_strdup_printf("\t\t<title>%s</title>",title);

	soup_message_body_append(body,SOUP_MEMORY_TAKE,title_,strlen(title_));
	soup_message_body_append(body,SOUP_MEMORY_STATIC,html_head_close,strlen(html_head_close));
}



void SoupServer_path_login(SoupServer *server, SoupMessage *msg,const char *path,
		GHashTable *query, SoupClientContext *client,gpointer user_data)
{

	if (strcmp(msg->method, "POST") == 0)
	{
		soup_message_set_status(msg, SOUP_STATUS_OK);

		soup_message_headers_set_content_type(msg->response_headers, "text/html",
				NULL);
		soup_message_headers_set_encoding(msg->response_headers,
				SOUP_ENCODING_CHUNKED);

		char id[32] =
		{ 0 };

		sscanf(msg->request_body->data, "id=%32[^&]", id);

		monitor_http_append_html_head(msg->response_body, "登录失败!");

		soup_message_body_append(msg->response_body, SOUP_MEMORY_STATIC,
				html_body_begin, strlen(html_body_begin));

		soup_message_body_append(msg->response_body, SOUP_MEMORY_STATIC,
				"登录识别，识别啊!你的 ID 是 ", strlen("登录识别，识别啊!你的 ID 是 "));

		soup_message_body_append(msg->response_body, SOUP_MEMORY_COPY, id,
				strlen(id));

		soup_message_body_append(msg->response_body, SOUP_MEMORY_STATIC,
				html_body_close, strlen(html_body_close));
		soup_message_body_append(msg->response_body, SOUP_MEMORY_STATIC,
				html_close, strlen(html_close));

		soup_message_body_complete(msg->response_body);
	}
}

void SoupServer_path_info(SoupServer *_server, SoupMessage *msg,
		const char *path, GHashTable *query, SoupClientContext *client,
		gpointer user_data)
{

	soup_message_set_status(msg,SOUP_STATUS_OK);

	soup_server_pause_message(_server,msg);

	soup_message_headers_set_content_type(msg->response_headers,"text/html",NULL);
	soup_message_headers_set_encoding(msg->response_headers,SOUP_ENCODING_CHUNKED);

	SoupMessageBody * body = msg->response_body ;

	gchar * title = g_strdup_printf("Info of the running %s , pid %d",PACKAGE_NAME,getpid());

	monitor_http_append_html_head(msg->response_body,title);

	g_free(title);

	gboolean monitor_http_append_info(gpointer _msg)
	{
		gchar * tr ;
		SoupMessage *msg = _msg;
		SoupMessageBody * body = msg->response_body;
		soup_message_body_append(body,SOUP_MEMORY_STATIC,html_body_begin,strlen(html_body_begin));
		//构造表格吧 :)

		tr = g_strdup_printf("<h1>Info of the running %s , pid %d<h1>",PACKAGE_NAME,getpid());
		soup_message_body_append(body,SOUP_MEMORY_TAKE,tr,strlen(tr));

		soup_message_body_append(body,SOUP_MEMORY_STATIC,"<dl>",strlen("<dl>"));

		tr = g_strdup_printf("<div id=\"info\"><dd>cpu usage %%%d</dd>",2);
		soup_message_body_append(body,SOUP_MEMORY_TAKE,tr,strlen(tr));


		soup_message_body_append(body,SOUP_MEMORY_STATIC,"</dl>",strlen("</dl>"));
		soup_message_body_append(body,SOUP_MEMORY_STATIC,html_body_close,strlen(html_body_close));
		soup_message_body_append(body,SOUP_MEMORY_STATIC,html_close,strlen(html_close));
		soup_message_body_complete(body);
		soup_server_unpause_message(server,msg);
		return FALSE;
	}
	g_idle_add(monitor_http_append_info,msg);
}

static void SoupServer_path_index(SoupServer *server, SoupMessage *msg,
		const char *path, GHashTable *query, SoupClientContext *client,
		gpointer user_data)
{
	soup_message_set_status(msg, SOUP_STATUS_OK);

	soup_message_headers_set_content_type(msg->response_headers, "text/html",
			NULL);
	soup_message_headers_set_encoding(msg->response_headers,
			SOUP_ENCODING_CHUNKED);

	monitor_http_append_html_head(msg->response_body, "登录以使用网络");

	soup_message_body_append(msg->response_body, SOUP_MEMORY_STATIC,
			html_body_begin, strlen(html_body_begin));

	const char * form =
			"\t<form action=\"/login.monitor\" method=\"POST\">\n";

	soup_message_body_append(msg->response_body, SOUP_MEMORY_STATIC, form,
			strlen(form));

	const char * table = "<table border=\"0\" align=\"center\" >\n<tr>\n\t\t";

	soup_message_body_append(msg->response_body, SOUP_MEMORY_STATIC,table, strlen(table));

	soup_message_body_append(msg->response_body, SOUP_MEMORY_STATIC,
			"<td>输入验证码:</td>", strlen("<td>输入验证码:</td>"));

	const char * input = "<td><input type=\"text\" name=\"id\"></input></td>";

	soup_message_body_append(msg->response_body, SOUP_MEMORY_STATIC, input,
			strlen(input));

	input = "<td><input type=\"submit\" value=\"验证\" name=\"Submit\"></input></td>";

	soup_message_body_append(msg->response_body, SOUP_MEMORY_STATIC, input,
			strlen(input));

	soup_message_body_append(msg->response_body, SOUP_MEMORY_STATIC,
			"\n</tr>\n</table>\n\n", strlen("\n</tr>\n</table>\n\n"));

	soup_message_body_append(msg->response_body, SOUP_MEMORY_STATIC,
			"\t</form>\n", strlen("\t</form>\n"));


	gchar * smsurl = g_key_file_get_string(gkeyfile, "http", "sms_url", NULL);

	if (smsurl)
	{

		form = g_strdup_printf(
				"<form action=\"%s\" method=\"GET\" target=\"smsframe\">\n",
				smsurl);

		soup_message_body_append(msg->response_body, SOUP_MEMORY_TAKE, form,
				strlen(form));

		soup_message_body_append(msg->response_body, SOUP_MEMORY_STATIC,table, strlen(table));


		soup_message_body_append(msg->response_body, SOUP_MEMORY_STATIC,
				"\t<td><input type=\"submit\" value=\"获取验证码\"></input></td>\n",
				strlen("\t<td><input type=\"submit\" value=\"获取验证码\"></input></td>\n"));

		soup_message_body_append(msg->response_body, SOUP_MEMORY_STATIC,
				"\n</tr>\n</table>\n\n<br>\n", strlen("\n</tr>\n</table>\n\n<br>\n"));

		soup_message_body_append(msg->response_body, SOUP_MEMORY_STATIC,
				"\t</form>\n", strlen("\t</form>\n"));

		const char * frame = "\n\t\t<iframe name=\"smsfrane\" height=\"80%\" width=\"100%\" src=\"%s\"> </iframe>";

		frame = g_strdup_printf(frame, smsurl);

		g_free(smsurl);

		soup_message_body_append(msg->response_body, SOUP_MEMORY_TAKE, frame,
				strlen(frame));

	}


	soup_message_body_append(msg->response_body, SOUP_MEMORY_STATIC,
			html_body_close, strlen(html_body_close));
	soup_message_body_append(msg->response_body, SOUP_MEMORY_STATIC,
			html_close, strlen(html_close));

	soup_message_body_complete(msg->response_body);


}


static void SoupServer_path_root(SoupServer *server, SoupMessage *msg,
		const char *path, GHashTable *query, SoupClientContext *client,
		gpointer user_data)
{
	static int i;


	if(g_strcmp0(path,"/")==0)
	{
		return SoupServer_path_index(server,msg,path,query,client,user_data);
	}
	soup_message_set_status(msg, SOUP_STATUS_OK);

	soup_message_headers_set_content_type(msg->response_headers, "text/html",
			NULL);
	soup_message_headers_set_encoding(msg->response_headers,
			SOUP_ENCODING_CHUNKED);

	HtmlNode * node = htmlnode_new(NULL, "html", NULL);

	htmlnode_new_text(htmlnode_new(htmlnode_new(node, "head", NULL), "title",
			NULL), "你好");

	HtmlNode * body = htmlnode_new(node, "body", NULL);

	HtmlNode * p = htmlnode_new(body,"p",NULL);



	char * bodytxt = g_strdup_printf(
			"你好 %d , 你访问的是 %s <br> POST 的是 %s", i++,
			path, msg->method);

	htmlnode_new_text(p, bodytxt);

	void appender(const gchar * txt, SoupMessageBody * body)
	{
		soup_message_body_append(body,SOUP_MEMORY_COPY,txt,strlen(txt));
	}

	htmlnode_to_plane_text_and_free(node,(htmlnode_appender)appender,msg->response_body);

	soup_message_body_complete(msg->response_body);
}
