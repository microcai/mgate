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
#include "http_server.h"
#include "i18n.h"
#include "global.h"
#include "monitor_icon.h"
#include "htmlnode.h"
#include "smsapi.h"

typedef struct{
	GTimer*timer;
	gchar phone[20];
	gchar code[10];
}phonetocode;

static void SoupServer_path_root_icon(SoupServer *server, SoupMessage *msg,
		const char *path, GHashTable *query, SoupClientContext *client,
		gpointer user_data);
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

//生成新的随机校验码
static gchar * genarate_new_code(const gchar * phonenumber);

static SoupServer * server;

static int cpu_usage;
static GList * phomecodemap;

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

static gboolean remove_outdated_phone_code_map(gpointer data)
{
	void remove_out_date(phonetocode* data, gpointer user_data)
	{
		if (g_timer_elapsed(data->timer, NULL) > GPOINTER_TO_INT(user_data))
		{
			g_debug("delete one\n");
			phomecodemap = g_list_remove(phomecodemap,data);
		}
	}
	g_list_foreach(phomecodemap,(GFunc)remove_out_date,data);

//	g_list_last(phomecodemap);
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
	soup_server_add_handler(server,"/favicon.ico",SoupServer_path_root_icon,NULL,NULL);

	soup_server_run_async(server);

	sms_init();

	g_timeout_add_seconds(10,celect_usage,NULL);

	g_timeout_add_seconds(1,remove_outdated_phone_code_map,GINT_TO_POINTER(g_key_file_get_integer(gkeyfile,"sms","validtime",0)));

	return soup_server_get_port(server);
}

void SoupServer_path_root_icon(SoupServer *server, SoupMessage *msg,
		const char *path, GHashTable *query, SoupClientContext *client,
		gpointer user_data)
{
	soup_message_set_status(msg, SOUP_STATUS_OK);
	soup_message_set_response(msg, "image/x-icon", SOUP_MEMORY_STATIC,
			(gpointer) _binary_favicon_ico_start, (long)_binary_favicon_ico_size);
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
			SOUP_ENCODING_CONTENT_LENGTH);

	HtmlNode * html = htmlnode_new(NULL,"html",NULL);

	HtmlNode * head = htmlnode_new_head(html, NULL);

	htmlnode_new(head,"link","rel=\"shortcut icon\"","href=\"/favicon.ico\"","type=\"image/x-icon\"",NULL);

	htmlnode_new_text(htmlnode_new(head,"title",NULL),"登录以使用网络");

	HtmlNode * body = htmlnode_new_body(html,NULL);

	HtmlNode * tr = htmlnode_new(htmlnode_new_table(htmlnode_new_form(body,"POST","/login.monitor",NULL),"border=\"0\"","align=\"center\"",NULL),"tr",NULL);

	htmlnode_new_text(htmlnode_new(htmlnode_new(tr,"td",NULL),"p",NULL),"输入验证码:");

	htmlnode_new(htmlnode_new(tr,"td",NULL),"input","type=\"text\"","name=\"id\"",NULL);


	htmlnode_new(htmlnode_new(tr,"td",NULL),"input","type=\"submit\"","value=\"验证\"","name=\"Submit\"",NULL);


	gchar * smsurl = g_key_file_get_string(gkeyfile, "http", "sms_url", NULL);

	if (smsurl)
	{
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
	gchar phonenumber[17]={0};

	char * genarated_code;

	soup_message_set_status(msg, SOUP_STATUS_OK);
	soup_message_headers_set_content_type(msg->response_headers, "text/html; charset=UTF-8",NULL);
	soup_message_headers_set_encoding(msg->response_headers,SOUP_ENCODING_CONTENT_LENGTH);

	html = htmlnode_new(NULL, "html", NULL);

	htmlnode_new_text(htmlnode_new(htmlnode_new_head(html,NULL),"title",NULL),"获取短信验证码");
	HtmlNode * body = htmlnode_new_body(html,NULL);

	if(g_strcasecmp(msg->method,"POST"))
	{
		//显示获取验证码表单

		HtmlNode * table = htmlnode_new_table(htmlnode_new_form(body,"POST","/getsmscode.asp",NULL),"border=\"0\"","align=\"center\"",NULL);
		htmlnode_new_text(htmlnode_new(htmlnode_new(table,"tr",NULL),"td",NULL),"输入手机号码:");
		htmlnode_new(htmlnode_new(htmlnode_new(table,"tr",NULL),"td",NULL),"input","type=\"text\"","name=\"phone\"",0);
		htmlnode_new(htmlnode_new(htmlnode_new(table,"tr",NULL),"td",NULL),"input","type=\"submit\"","value=\"获取验证码\"","name=\"Submit\"",NULL);

	}else
	{
		sscanf(msg->request_body->data,"phone=%16[+0123456789]",phonenumber);
		phonenumber[16]=0;

		genarated_code = genarate_new_code(phonenumber);

		htmlnode_new_text(htmlnode_new(htmlnode_new(htmlnode_new_table(body,"align=\"center\"",NULL),"tr",NULL),"td",NULL),msg->request_body->data);

		if(genarated_code)
		{
			htmlnode_new_text(htmlnode_new(htmlnode_new(htmlnode_new_table(body,"align=\"center\"",NULL),"tr",NULL),"td",NULL),"一份包含有验证码的短信已发送，请注意查收");
		}else //报告系统忙
		{
			htmlnode_new_text(htmlnode_new(htmlnode_new(htmlnode_new_table(body,"align=\"center\"",NULL),"tr",NULL),"td",NULL),_("System busy, try later!"));
		}

		g_free(genarated_code);
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

	HtmlNode * head = htmlnode_new_head(node, NULL);

	htmlnode_new(head,"link","rel=\"shortcut icon\"","href=\"/favicon.ico\"","type=\"image/x-icon\"",NULL);

	htmlnode_new_text(htmlnode_new(head, "title",NULL), "你好");

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

static gchar * build_message_text(const gchar * msgcode)
{
	gchar * template = g_key_file_get_string(gkeyfile,"sms","template",NULL);

	if(!template)
		return g_strdup(msgcode);

	gint validtime_int = g_key_file_get_integer(gkeyfile,"sms","validtime",0);

	gchar * validtime = g_strdup_printf("%d 分钟",validtime_int/60);

	//开始做字符串替换，使用正则表达式啊
	GRegex * regcode = g_regex_new("%code",0,0,NULL);
	GRegex * regvalidtime = g_regex_new("%validtime",0,0,NULL);

	gchar * replacedcode = g_regex_replace_literal(regcode,template,-1,0,msgcode,0,0);

	g_regex_unref(regcode);

	gchar * replacedvalidtime =  g_regex_replace_literal(regvalidtime,replacedcode,-1,0,validtime,0,0);

	g_free(validtime);
	g_free(replacedcode);
	g_regex_unref(regvalidtime);

	return replacedvalidtime;
}

static gchar * genarate_new_code(const gchar * phonenumber)
{
	gchar * msgcode = NULL;

	if(strlen(phonenumber)>10)
	{
		int code = lrand48() % 99999;

		msgcode = g_strdup_printf("%06" G_GINT32_MODIFIER "d" ,code);

		gchar * msg = build_message_text(msgcode);

		sms_sendmessage(phonenumber,msg);

		phonetocode * ph2code = g_new0(phonetocode,1);

		ph2code->timer = g_timer_new();

		strcpy(ph2code->phone,phonenumber);
		strcpy(ph2code->code,msgcode);

		phomecodemap = g_list_prepend(phomecodemap,ph2code);

		g_debug("out messge for phone %s is %s",phonenumber,msg);

	}return msgcode;
}
