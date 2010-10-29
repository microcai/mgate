/*
 * html_path_get_code.c
 *
 *  Created on: 2010-10-24
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
#include "html_paths.h"
#include "g_socket_source.h"
#include "smsserver_connector.h"

static SoupServer *server;

enum SmscenterStatusCode{
	SERVER_NO_RESPONSE,
	SERVER_LOGIN_FAILED,
	SERVER_CODE_GENERATED,
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
void send_getcode(smsserver_result* rst, SoupMessage *msg,const char *path,GHashTable *query, SoupClientContext *client,gpointer user_data)
{
	soup_message_set_status(msg,SOUP_STATUS_BAD_REQUEST);

	soup_message_set_response(msg,"text/html",SOUP_MEMORY_STATIC,"服务器内部错误",21);

	return 	soup_server_unpause_message(server,msg);
}
//向远程server发起请求一个鉴证码
void SoupServer_path_getverifycode(SoupServer *_server, SoupMessage *msg,
		const char *path, GHashTable *query, SoupClientContext *client,
		gpointer user_data)
{
	static gchar * smshost;

	server = _server;

	if(!smshost)
	{
		smshost = g_key_file_get_string(gkeyfile,"sms","smshost",0);
		smshost = g_strstrip(smshost);
	}

	if(!smshost)
	{//没设置？！
		g_error("[sms]:gencode 没设置，肏，快设置");
	}

	if(!smsserver_is_online())
	{
		//大哥，不在线啊，诶，直接放行好了。
		return SoupServer_path_static_file(server,msg,path,query,client,user_data);
	}

	//先暂停掉服务器
	soup_message_set_status(msg,SOUP_STATUS_BAD_REQUEST);
	soup_server_pause_message(server,msg);

	//发起异步连接到短信服务器
	smsserver_getcode(send_getcode,msg,path,query,client,user_data);
}
