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

gboolean  sms_read_ready(GSocket * socket, GIOCondition cond , SoupMessage *msg)
{
	char buff[256];
	gssize ret = g_socket_receive(socket,buff,sizeof(buff),0,0);

	if(ret)
	{

	}

	SoupServer_path_static_file(server,msg,"/gotpswd.html",0,0,0);
	return FALSE;
}

void sms_host_connected(GSocketClient *source_object, GAsyncResult *res,SoupMessage *msg)
{
	static gchar * clientid;

	if(!clientid)
	{
		clientid = g_key_file_get_string(gkeyfile,"kpolice","HotelID",0);
		clientid = g_strstrip(clientid);
	}

	GSocketConnection * connection = g_socket_client_connect_to_host_finish(source_object,res,NULL);
	g_object_unref(source_object);

	if(!connection)
		return make_response(msg,SERVER_NO_RESPONSE,0);

	GSocket * sock = g_object_ref(g_socket_connection_get_socket(connection));

	gchar * usr = g_strdup_printf("USER %s\n\n",clientid);

	g_socket_send(sock,usr,strlen(usr),0,0);

	g_socket_add_watch(sock,G_IO_IN,(GSockIOFunc)sms_read_ready,msg);

	g_free(usr);
	return ;
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
	GSocketClient * clienter = g_socket_client_new();
	g_socket_client_connect_to_host_async(clienter,smshost,25720,0,(GAsyncReadyCallback)sms_host_connected,msg);
}
