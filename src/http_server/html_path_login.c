/*
 * html_path_login.c
 *
 *  Created on: 2010-10-22
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
#include "monitor_icon.h"
#include "htmlnode.h"
#include "smsapi.h"
#include "html_paths.h"

static GList * phomecodemap;


void SoupServer_path_login(SoupServer *server, SoupMessage *msg,const char *path,
		GHashTable *query, SoupClientContext *client,gpointer user_data)
{
	guchar mac[6];
	const gchar * ip = soup_client_context_get_host(client);

	int sockclient = soup_socket_get_fd(soup_client_context_get_socket(client));

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

		sscanf(msg->request_body->data, "id=%31[0123456789]", id);

		HtmlNode * body = htmlnode_new_body(html,NULL);

		HtmlNode * p = htmlnode_new(body,"p",NULL);

		gboolean	find_same_id(phonetocode* data , gchar * code)
		{
			return strcasecmp(data->code,code);
		}

		GList * founded = g_list_find_custom(phomecodemap,id,(GCompareFunc)find_same_id);

		if(founded && arp_ip2mac(inet_addr(ip),mac,sockclient))
		{
			Client * client = client_new(((phonetocode*)founded->data)->phone,((phonetocode*)founded->data)->phone,"990",mac);
			g_object_set(client,"ipstr", ip, "enable",TRUE,NULL);
			clientmgr_insert_client_by_mac(mac,client);

			htmlnode_new_text(p,"手机号:");
			htmlnode_new_text(p,((phonetocode*)founded->data)->phone);
			htmlnode_new_text(p,"登录成功，你现在起可以自由访问网络了:)");
			htmlnode_new_text(htmlnode_new(body,"p",NULL),"如果您长时间没有网络连接，只需要重新认证就可以了，就这么简单:)");
			htmlnode_new_text(htmlnode_new(htmlnode_new_head(html,NULL),"title",NULL),"登录成功!");
		}else
		{
			htmlnode_new_text(htmlnode_new(htmlnode_new_head(html,NULL),"title",NULL),"登录失败!");
			htmlnode_new_text(p,"登录失败，失败啊!你的 ID 是 ");
			htmlnode_new_text(p,id);
		}
		/*
		gchar macstr[55];
		formatMAC(mac,macstr);
		g_debug("ip is %s, mac is %6s\n",ip,macstr);
		*/

		htmlnode_to_plane_text_and_free(html,(htmlnode_appender)soup_message_body_appender,msg->response_body);
		soup_message_body_complete(msg->response_body);
	}
}

//生成新的随机校验码
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


gboolean remove_outdated_phone_code_map(gpointer data)
{
	void remove_out_date(phonetocode* data, gpointer user_data)
	{
		if (g_timer_elapsed(data->timer, NULL) > GPOINTER_TO_INT(user_data))
		{
			g_timer_destroy(data->timer);
			phomecodemap = g_list_remove(phomecodemap,data);
			g_free(data);
		}
	}
	g_list_foreach(phomecodemap,(GFunc)remove_out_date,data);
	return TRUE;
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
#ifdef DEBUG
		htmlnode_new_text(htmlnode_new(htmlnode_new(htmlnode_new_table(body,"align=\"center\"",NULL),"tr",NULL),"td",NULL),msg->request_body->data);
#endif
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


