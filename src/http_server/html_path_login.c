/*
 * html_path_login.c -- 短信验证登录功能
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
#include "clientmgr.h"
#include "utils.h"
#include "http_server.h"
#include "global.h"
#include "htmlnode.h"
#include "html_paths.h"
#include "smsserver_connector.h"
#include "unzip.h"
#include "ksql.h"

static void xml_meet_text(GMarkupParseContext *context, const gchar *text,
		gsize text_len, gpointer user_data, GError **error);

static GMarkupParser parser ={
		.text = xml_meet_text
};

static void xml_meet_text(GMarkupParseContext *context, const gchar *text,
		gsize text_len, gpointer user_data, GError **error)
{
	if(!g_strcmp0(g_markup_parse_context_get_element(context),user_data))
		strncpy(user_data,text,29);
}


static void sms_verify_code_ready(smsserver_result* rst, SoupServer *server,SoupMessage *msg,const char *path, GHashTable *query, SoupClientContext *client,gpointer user_data)
{
	guchar mac[6];
	gchar	 phone[31]="phone";
	GMarkupParseContext * context;
	const gchar * ip;

	ip = soup_client_context_get_host(client);

	int sockclient = soup_socket_get_fd(soup_client_context_get_socket(client));

	soup_server_unpause_message(server,msg);
	soup_message_set_status(msg, SOUP_STATUS_OK);

	soup_message_headers_set_content_type(msg->response_headers, "text/html; charset=UTF-8",
			NULL);
	soup_message_headers_set_encoding(msg->response_headers,
			SOUP_ENCODING_CONTENT_LENGTH);

	HtmlNode * html = htmlnode_new(NULL,"html",NULL);

	HtmlNode *head =  htmlnode_new_head(html,NULL);

	htmlnode_new_jssrc(head,"jquery-1.4.3.js");
	htmlnode_new_jssrc(head,"jheartbeat.js");

	HtmlNode * body = htmlnode_new_body(html,NULL);

	if(!rst) //需要根据服务器的结果生成通知页面了，(*^__^*) 嘻嘻……
	{
		return SoupServer_path_static_file(server,msg,"/err",query,client,user_data);
	}
	switch (rst->statuscode)
	{
		case SOUP_STATUS_OK: //恩，好
		{
			//首先解析出 phone 号码
			context = g_markup_parse_context_new(&parser,0,phone,0);
			g_markup_parse_context_parse(context,rst->buffer,rst->length,0);
			g_markup_parse_context_free(context);
			HtmlNode * p = htmlnode_new(body,"p",NULL);

			if(arp_ip2mac(inet_addr(ip),mac,sockclient))
			{
				Client * client = client_new(phone,phone,"990",mac);
				g_object_set(client,"ipstr", ip, "enable",TRUE,NULL);

				//TODO ; check if it is self owned computer
				client->remove_outdate = TRUE;

				clientmgr_insert_client_by_mac(mac,client);

				ksql_vquery_async(
						"insert into roomer_list (RoomId,CustomerName,IDtype,ID,IP_ADDR,MAC_ADDR) values(0,'%s','%s','%s','%s','%s') ",
						phone,"990",phone,ip,mac);


				HtmlNode * div = htmlnode_new(body,"div","id=\"test_div\"",0);

				p = htmlnode_new(div,"p",NULL);

				if(smsserver_is_online())
					htmlnode_new_text(p,"手机号:");
				else
					htmlnode_new_text(p,"IP地址:");
				htmlnode_new_text(p,phone);
				htmlnode_new_text(p,"登录成功，你现在起可以自由访问网络了:)");
				htmlnode_new_text(htmlnode_new(div,"p",NULL),"请不要关闭本页。如果您关闭了本页面，您将立即断网");
				htmlnode_new_text(htmlnode_new(htmlnode_new_head(html,NULL),"title",NULL),"登录成功!");

				gchar * keep_aliveurl = g_strdup_printf("/keep_alive?phone=%s",phone);

				htmlnode_new_iframe(body,keep_aliveurl,"height=\"-1\"","width=\"-1\"",0);

				g_free(keep_aliveurl);

			}else
			{
				htmlnode_new_text(htmlnode_new(htmlnode_new_head(html,NULL),"title",NULL),"登录失败!");
				htmlnode_new_text(p,"登录失败，请在局域网环境中使用!你的 手机 是 ");
				htmlnode_new_text(p,phone);
			}
		}
		break;
		case SOUP_STATUS_PROCESSING:
		{
			return SoupServer_path_static_file(server,msg,"/processing.html",query,client,user_data);
		}break;
		case SOUP_STATUS_NOT_FOUND: //验证码没发现
		{
			return SoupServer_path_static_file(server,msg,"/err",query,client,user_data);

			// 没发现啊没发现，
		}break;
		default: //显示失败信息，要求重试
			break;
	}

	htmlnode_to_plane_text_and_free(html,(htmlnode_appender)soup_message_body_appender,msg->response_body);
	soup_message_body_complete(msg->response_body);
}

static void sms_getcode_response(smsserver_result* rst,SoupServer *server, SoupMessage *msg,const char *path,GHashTable *query, SoupClientContext *client,gpointer user_data)
{
	extern const char _binary_resource_zip_start[];
	extern const char _binary_resource_zip_end[];

	char code[30]="code";
	char smsc[30]="smsc";

	GMarkupParseContext * context;

	if (!rst || rst->statuscode!=200)
	{
		//连接错误
		SoupServer_path_static_file(server,msg,path,query,client,user_data);
		soup_message_set_status(msg, SOUP_STATUS_BAD_REQUEST);
	}
	else
	{
		soup_message_set_status(msg,rst->statuscode);

		//接下来，进行 XML 解析来搞到文本，(*^__^*) 嘻嘻……
		context = g_markup_parse_context_new(&parser,0,code,0);
		g_markup_parse_context_parse(context,rst->buffer,rst->length,0);
		g_markup_parse_context_free(context);

		context = g_markup_parse_context_new(&parser,0,smsc,0);
		g_markup_parse_context_parse(context,rst->buffer,rst->length,0);
		g_markup_parse_context_free(context);
		//然后根据搞到的验证码，生成页面
		//通过解压内置的 resource.zip 来获得模板。文件名是 getcode.xml
		const zipRecord * ziprc = zipbuffer_search(_binary_resource_zip_start,_binary_resource_zip_end,"getcode.xml");
		char dst[ziprc->size_orig];
		gsize dstlen  =  sizeof(dst);
		unzip_buffer(dst,&dstlen,ziprc);
		dst[dstlen]=0;
		gchar * html = g_strdup_printf(dst,code,code,smsc);
		soup_message_body_append(msg->response_body,SOUP_MEMORY_TAKE,html,strlen(html));
		soup_message_headers_set_content_type(msg->response_headers,"text/html",0);

		soup_message_body_complete(msg->response_body);
	}
	return 	soup_server_unpause_message(server,msg);
}

//向远程server请求一个鉴证码对应的手机号码
void SoupServer_path_login(SoupServer *server, SoupMessage *msg,const char *path,
		GHashTable *query, SoupClientContext *client,gpointer user_data)
{
	char code[32] =
	{ 0 };

	if (g_strcmp0(msg->method, "POST"))
		return ;

	if (smsserver_is_online())
	{
		sscanf(msg->request_body->data, "codename=%31[0123456789]", code);
		//这个时候我们应该向服务器发起认证，获得对应的手机号码
		soup_server_pause_message(server,msg);

		return smsserver_verifycode(sms_verify_code_ready,code,server,msg,path,query,client,user_data);
	}else
	{
		//可以直接上网了，呵呵
		smsserver_result rst;

		rst.statuscode = 200;

		rst.buffer = g_strdup_printf("<html><phone>%s</phone></html>",soup_client_context_get_host(client));
		rst.length = strlen(rst.buffer);
		sms_verify_code_ready(&rst,server,msg,path,query,client,user_data);
		g_free((gpointer)rst.buffer);
	}
}

//向远程server发起请求一个鉴证码
void SoupServer_path_getverifycode(SoupServer *server, SoupMessage *msg,
		const char *path, GHashTable *query, SoupClientContext *client,
		gpointer user_data)
{
	if(!smsserver_is_online())
	{
		//大哥，不在线啊，诶，直接放行好了。
		return SoupServer_path_static_file(server,msg,path,query,client,user_data);
	}

	//先暂停掉服务器
	soup_message_set_status(msg,SOUP_STATUS_BAD_REQUEST);
	soup_server_pause_message(server,msg);
	//发起异步连接到短信服务器
	smsserver_getcode(sms_getcode_response,server,msg,path,query,client,user_data);
}
