/*
 * html_path_get_code.c -- 实现短信验证
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
#include <libsoup/soup.h>
#include "clientmgr.h"
#include "utils.h"
#include "http_server.h"
#include "i18n.h"
#include "global.h"
#include "unzip.h"
#include "html_paths.h"
#include "smsserver_connector.h"

static SoupServer *server;

enum SmscenterStatusCode{
	SERVER_NO_RESPONSE,
	SERVER_LOGIN_FAILED,
	SERVER_CODE_GENERATED,
};

static void xml_meet_text(GMarkupParseContext *context, const gchar *text,
		gsize text_len, gpointer user_data, GError **error);

static GMarkupParser parser ={
		.text = xml_meet_text
};

/*

static void make_response(SoupMessage * msg, int smscenter_status_code,const char * code)
{
	soup_server_unpause_message(server,msg);
	switch (smscenter_status_code)
	{
		case SERVER_NO_RESPONSE:
			//可以直接上网了
			return SoupServer_path_static_file(server,msg,"/goahead.html",0,0,0);
			break;
		default:
			break;
	}

}
*/
static void xml_meet_text(GMarkupParseContext *context, const gchar *text,
		gsize text_len, gpointer user_data, GError **error)
{
	if(!g_strcmp0(g_markup_parse_context_get_element(context),user_data))
		strncpy(user_data,text,29);
}

static void sms_getcode_response(smsserver_result* rst, SoupMessage *msg,const char *path,GHashTable *query, SoupClientContext *client,gpointer user_data)
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

//向远程server发起请求一个鉴证码
void SoupServer_path_getverifycode(SoupServer *_server, SoupMessage *msg,
		const char *path, GHashTable *query, SoupClientContext *client,
		gpointer user_data)
{
	server = _server;
	if(!smsserver_is_online())
	{
		//大哥，不在线啊，诶，直接放行好了。
		return SoupServer_path_static_file(server,msg,path,query,client,user_data);
	}

	//先暂停掉服务器
	soup_message_set_status(msg,SOUP_STATUS_BAD_REQUEST);
	soup_server_pause_message(server,msg);
	//发起异步连接到短信服务器
	smsserver_getcode(sms_getcode_response,msg,path,query,client,user_data);
}

static void sms_verifycode_response(smsserver_result* rst, SoupMessage *msg,const char *path,GHashTable *query, SoupClientContext *client,gpointer user_data)
{
	soup_server_unpause_message(server,msg);




}

//向远程server发起请求鉴证获得对应的返回值，(*^__^*) 嘻嘻……
void SoupServer_path_verifycode(SoupServer *_server, SoupMessage *msg,
		const char *path, GHashTable *query, SoupClientContext *client,
		gpointer user_data)
{
	server = _server;
	if(!smsserver_is_online())
	{
		//TODO:大哥，不在线啊!
	}
	//先暂停掉服务器
	soup_message_set_status(msg,SOUP_STATUS_BAD_REQUEST);
	soup_server_pause_message(server,msg);
	//发起异步连接到短信服务器
	smsserver_verifycode(sms_verifycode_response,g_hash_table_lookup(query,"code"),msg,path,query,client,user_data);
}
