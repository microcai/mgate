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
static void SoupServer_path_getsmscode(SoupServer *_server, SoupMessage *msg,
		const char *path, GHashTable *query, SoupClientContext *client,
		gpointer user_data);
static void SoupServer_path_login(SoupServer *server, SoupMessage *msg,
		const char *path, GHashTable *query, SoupClientContext *client,
		gpointer user_data);
static void soup_message_body_appender(const gchar * txt, SoupMessageBody * body);

static SoupServer * server;

static int cpu_usage;

static gboolean celect_usage(gpointer msg)
{
	static struct rusage usage;
	struct rusage cur_usage;

	getrusage(RUSAGE_SELF,&cur_usage);

	cpu_usage = ((cur_usage.ru_utime.tv_sec + cur_usage.ru_stime.tv_sec - usage.ru_utime.tv_sec -usage.ru_stime.tv_sec)*1000

		+ ( cur_usage.ru_utime.tv_usec + cur_usage.ru_stime.tv_usec -  usage.ru_utime.tv_usec - usage.ru_stime.tv_usec)/1000

	)/(100);

	usage = cur_usage;

	return TRUE;
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

	soup_server_add_handler(server,"/login",SoupServer_path_login,NULL,NULL);

	soup_server_add_handler(server,"/",SoupServer_path_root,NULL,NULL);
	soup_server_add_handler(server,"/index.html",SoupServer_path_index,NULL,NULL);
	soup_server_add_handler(server,"/index.htm",SoupServer_path_index,NULL,NULL);
	soup_server_add_handler(server,"/info",SoupServer_path_info,NULL,NULL);

	soup_server_add_handler(server,"/getsmscode.asp",SoupServer_path_getsmscode,NULL,NULL);

	void SoupServer_path_root_icon(SoupServer *server, SoupMessage *msg,
			const char *path, GHashTable *query, SoupClientContext *client,
			gpointer user_data)
	{
		soup_message_set_status(msg, SOUP_STATUS_OK);
		soup_message_set_response(msg,"image/x-icon",SOUP_MEMORY_STATIC,
				(gpointer)monitor_icon,sizeof(monitor_icon));
	}

	soup_server_add_handler(server,"/favicon.ico",SoupServer_path_root_icon,NULL,NULL);

	soup_server_run_async(server);

	g_timeout_add_seconds(10,celect_usage,NULL);
//
//	gboolean id(gpointer a)
//	{
//		return TRUE;
//
//	}
//
//	g_timeout_add(1,id,NULL);

	return soup_server_get_port(server);
}

void SoupServer_path_login(SoupServer *server, SoupMessage *msg,const char *path,
		GHashTable *query, SoupClientContext *client,gpointer user_data)
{

	if (strcmp(msg->method, "POST") == 0)
	{
		soup_message_set_status(msg, SOUP_STATUS_OK);

		soup_message_headers_set_content_type(msg->response_headers, "text/html; charset=UTF-8",
				NULL);
		soup_message_headers_set_encoding(msg->response_headers,
				SOUP_ENCODING_CHUNKED);

		HtmlNode * html = htmlnode_new(NULL,"html",NULL);

		char id[32] =
		{ 0 };

		sscanf(msg->request_body->data, "id=%32[^&]", id);

		htmlnode_new_text(htmlnode_new(htmlnode_new_head(html,NULL),"title",NULL),"登录失败!");

		HtmlNode * body = htmlnode_new_body(html,NULL);

		HtmlNode * p = htmlnode_new(body,"p",NULL);

		htmlnode_new_text(p,"登录识别，识别啊!你的 ID 是 ");
		htmlnode_new_text(p,id);

		htmlnode_to_plane_text_and_free(html,(htmlnode_appender)soup_message_body_appender,msg->response_body);
		soup_message_body_complete(msg->response_body);
	}
}

void SoupServer_path_info(SoupServer *_server, SoupMessage *msg,
		const char *path, GHashTable *query, SoupClientContext *client,
		gpointer user_data)
{
	SoupMessageBody * body = msg->response_body;

	soup_message_set_status(msg,SOUP_STATUS_OK);

	soup_server_pause_message(_server,msg);

	soup_message_headers_set_content_type(msg->response_headers,"text/html",NULL);
	soup_message_headers_set_encoding(msg->response_headers,SOUP_ENCODING_CHUNKED);

	HtmlNode * html = htmlnode_new(NULL, "html", NULL);

	gchar * title = g_strdup_printf("Info of the running %s , pid %d",
			PACKAGE_NAME, getpid());

	htmlnode_new_text(htmlnode_new(htmlnode_new_head(html, NULL), "title", NULL), title);

	g_free(title);

	gchar * tr;

	//构造表格吧 :)

	tr = g_strdup_printf("Info of the running %s , pid %d", PACKAGE_NAME,
			getpid());

	HtmlNode * htmlbody = htmlnode_new_body(html, NULL);

	htmlnode_new_text(htmlnode_new(htmlbody, "h1", NULL), tr);

	g_free(tr);

	HtmlNode * div = htmlnode_new(htmlbody, "div", "id=\"info\"", NULL);


	tr = g_strdup_printf("cpu usage %%%d",cpu_usage);

	htmlnode_new_text(htmlnode_new(div, "dd", NULL), tr);

	g_free(tr);

	htmlnode_to_plane_text_and_free(html,
			(htmlnode_appender) soup_message_body_appender, msg->response_body);

	soup_message_body_complete(body);
	soup_server_unpause_message(server, msg);

}

static void SoupServer_path_index(SoupServer *server, SoupMessage *msg,
		const char *path, GHashTable *query, SoupClientContext *client,
		gpointer user_data)
{
	soup_message_set_status(msg, SOUP_STATUS_OK);

	soup_message_headers_set_content_type(msg->response_headers, "text/html; charset=UTF-8",
			NULL);
	soup_message_headers_set_encoding(msg->response_headers,
			SOUP_ENCODING_CHUNKED);

	HtmlNode * html = htmlnode_new(NULL,"html",NULL);

	htmlnode_new_text(htmlnode_new(htmlnode_new_head(html,NULL),"title",NULL),"登录以使用网络");

	HtmlNode * body = htmlnode_new_body(html,NULL);

	HtmlNode * tr = htmlnode_new(htmlnode_new_table(htmlnode_new_form(body,"POST","/login.monitor",NULL),"border=\"0\"","align=\"center\"",NULL),"tr",NULL);

	htmlnode_new_text(htmlnode_new(htmlnode_new(tr,"td",NULL),"p",NULL),"输入验证码:");

	htmlnode_new(htmlnode_new(tr,"td",NULL),"input","type=\"text\"","name=\"id\"",NULL);


	htmlnode_new(htmlnode_new(tr,"td",NULL),"input","type=\"submit\"","value=\"验证\"","name=\"Submit\"",NULL);


	gchar * smsurl = g_key_file_get_string(gkeyfile, "http", "sms_url", NULL);

	if (smsurl)
	{
		HtmlNode * form = htmlnode_new_form(body,"GET",smsurl,"targe=\"smsframe\"",NULL);

		HtmlNode * tr = htmlnode_new(htmlnode_new_table(form,"border=\"0\"","align=\"center\"",NULL),"tr",NULL);


		htmlnode_new(htmlnode_new(tr,"td",NULL),"input","type=\"submit\"","value=\"获取验证码\"",NULL);

		htmlnode_new_iframe(body,smsurl,"height=\"80%\"","width=\"100%\"","name=\"smsframe\"",NULL);

		g_free(smsurl);

	}
	htmlnode_to_plane_text_and_free(html,(htmlnode_appender)soup_message_body_appender,msg->response_body);

	soup_message_body_complete(msg->response_body);
}

void SoupServer_path_getsmscode(SoupServer *_server, SoupMessage *msg,
		const char *path, GHashTable *query, SoupClientContext *client,
		gpointer user_data)
{
	HtmlNode	* html;

	char * genarated_code = NULL;//"124";

	soup_message_set_status(msg, SOUP_STATUS_OK);
	soup_message_headers_set_content_type(msg->response_headers, "text/html; charset=UTF-8",NULL);
	soup_message_headers_set_encoding(msg->response_headers,SOUP_ENCODING_CONTENT_LENGTH);

	html = htmlnode_new(NULL, "html", NULL);

	htmlnode_new_text(htmlnode_new(htmlnode_new_head(html,NULL),"title",NULL),"获取短信验证码");
	HtmlNode * body = htmlnode_new_body(html,NULL);
	if(genarated_code)
	{
		htmlnode_new_text(htmlnode_new(htmlnode_new(htmlnode_new_table(body,"align=\"center\"",NULL),"tr",NULL),"td",NULL),genarated_code);
	}else //报告系统忙
	{
		htmlnode_new_text(htmlnode_new(htmlnode_new(htmlnode_new_table(body,"align=\"center\"",NULL),"tr",NULL),"td",NULL),_("System busy, try later!"));
	}
	htmlnode_to_plane_text_and_free(html,(htmlnode_appender)soup_message_body_appender,msg->response_body);
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

	soup_message_headers_set_content_type(msg->response_headers, "text/html; charset=UTF-8",
			NULL);
	soup_message_headers_set_encoding(msg->response_headers,
			SOUP_ENCODING_CHUNKED);

	HtmlNode * node = htmlnode_new(NULL, "html", NULL);

	htmlnode_new_text(htmlnode_new(htmlnode_new(node, "head", NULL), "title",
			NULL), "你好");

	HtmlNode * p = htmlnode_new(htmlnode_new(node, "body", NULL),"p",NULL);

	char * bodytxt = g_strdup_printf(
			"你好 %d , 你访问的是 %s <br> POST 的是 %s", i++,
			path, msg->method);

	htmlnode_new_text(p, bodytxt);

	g_free(bodytxt);


	htmlnode_to_plane_text_and_free(node,(htmlnode_appender)soup_message_body_appender,msg->response_body);

	soup_message_body_complete(msg->response_body);
}

static void soup_message_body_appender(const gchar * txt, SoupMessageBody * body)
{
	soup_message_body_append(body,SOUP_MEMORY_COPY,txt,strlen(txt));
}
