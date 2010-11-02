/*
 * smsserver_connector.c -- 短信服务器客户端
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
#include "global.h"
#include "smsserver_connector.h"
#include "socket/g_socket_client_proxy.h"
#include "md5.h"

typedef struct smscbdata{
	smsserver_readycallback cb;
	gpointer data;
	SoupServer *server;
	SoupMessage * msg;
	const char *path;
	GHashTable *query;
	SoupClientContext *client;
	GSocketConnection * connec;
	gchar	code[30]; // set for verify
	gchar	readbuffer[512];
}smscbdata;

#define CALL_USER_CB(psmscbdata,pstatus)	\
	psmscbdata->cb(pstatus,psmscbdata->server,psmscbdata->msg,psmscbdata->path,\
		psmscbdata->query,psmscbdata->client,psmscbdata->data)



static GSocketClient	* connector;
static gchar *			  smshost;
static gushort			  smsport = 25720;
static gchar * 			user_login;
static gboolean			isonline ;
static gchar * 			getcode;
static gchar * 			verify_code;
static const gchar *				passwd = "123456";

static gint loop_connect;

static void smsserver_loop_connected(GSocketClient *source_object,GAsyncResult *res, smscbdata* user_data)
{
	loop_connect = 0;

	GSocketConnection * connec =  g_socket_client_connect_to_host_finish(source_object,res,NULL);

	isonline = (connec!=NULL);

#ifdef DEBUG
	g_debug("短信服务器 %s 目前%s",smshost,isonline?"在线":"不在线");
#endif
}

static gboolean lets_loop_connect(gpointer user_data)
{
	if(!loop_connect)
	{
		loop_connect =1 ;
		g_socket_client_connect_to_host_async(user_data,smshost,smsport,0,(GAsyncReadyCallback)smsserver_loop_connected,NULL);
	}
#ifdef DEBUG
	else{ g_debug("重连的时候还没超时"); }
#endif
	return TRUE;
}

void smsserver_pinger_start()
{
	//获得 smshost
	//开始空白连接，哦哈哈
	gchar * hid=g_strstrip(g_key_file_get_string(gkeyfile,"kpolice","HotelID",0));
	user_login = g_strdup_printf("USER %s\n\n",hid);
	getcode = g_strdup_printf("GET /gencode?ID=%s\n\n",hid);
	verify_code = g_strdup_printf("GET /verifycode?ID=%s",hid);
	g_free(hid);

	smshost = g_strstrip(g_key_file_get_string(gkeyfile,"sms","smshost",0));

	connector = g_socket_client_new();

	lets_loop_connect(connector);

	//开始不停的连接吧，哈哈
	g_timeout_add_seconds(5,lets_loop_connect,connector);
}

gboolean smsserver_is_online()
{
	return isonline;
}

static void smsserver_recv_getX_ready(GInputStream *source_object,GAsyncResult *res, smscbdata* user_data)
{
	gssize ret = g_input_stream_read_finish(source_object,res,0);

	if(ret <=0)
	{
		CALL_USER_CB(user_data,0);
		g_object_unref(user_data->connec);
		g_slice_free(smscbdata,user_data);
		return ;
	}
	gint	status;
	//	好了，我们读取返回的东西了。读取第一行先
	sscanf(user_data->readbuffer,"%d %*s\n\n",&status);

	switch(status)
	{
	case SOUP_STATUS_OK:
	case SOUP_STATUS_NOT_FOUND:
	case SOUP_STATUS_PROCESSING:
		break;
	default:
		CALL_USER_CB(user_data,0);
		g_object_unref(user_data->connec);
		g_slice_free(smscbdata,user_data);
		return ;
	}

	smsserver_result rst;
	rst.statuscode = status;

	//寻找一个 "\n\n"
	char * line = strstr(user_data->readbuffer,"\n\n");
	if(line)
	{
		line += 2;

		rst.buffer = line;
		rst.length = ret - (line - user_data->readbuffer);

		CALL_USER_CB(user_data,&rst);
	}else
	{
		CALL_USER_CB(user_data,0);
	}

	g_object_unref(user_data->connec);
	g_slice_free(smscbdata,user_data);
}

static void smsserver_send_getX_ready(GOutputStream *source_object,GAsyncResult *res, smscbdata* user_data)
{
	gssize ret = g_output_stream_write_finish(source_object,res,0);

	if(ret<=0)
	{
		CALL_USER_CB(user_data,0);
		g_object_unref(user_data->connec);
		g_slice_free(smscbdata,user_data);
		return ;
	}
	memset(user_data->readbuffer,0,sizeof(user_data->readbuffer));
	g_input_stream_read_async(g_io_stream_get_input_stream(G_IO_STREAM(user_data->connec)),
			user_data->readbuffer,sizeof(user_data->readbuffer),0,0,(GAsyncReadyCallback)smsserver_recv_getX_ready,user_data);
}

static void smsserver_send_ready(GOutputStream *source_object,GAsyncResult *res, smscbdata* user_data);

static void smsserver_recv_user_login_ready(GInputStream *source_object,GAsyncResult *res, smscbdata* user_data)
{
	char * seed;
	u_char	digest[16];
	gchar * compass;

	gssize ret = g_input_stream_read_finish(source_object,res,0);

	if(ret <=0)
	{
		CALL_USER_CB(user_data,0);
		g_object_unref(user_data->connec);
		g_slice_free(smscbdata,user_data);
		return ;
	}
	//解析服务器返回内容。当下为一个 200 OK
	gint	status;
	sscanf(user_data->readbuffer,"%d %*s",&status);
	switch (status)
	{
		case 200: //恩，可以开始发送Login 代码了，
			if(!user_data->code[0])
			{
				//get code
				g_output_stream_write_async(g_io_stream_get_output_stream(G_IO_STREAM(user_data->connec)),
					getcode,strlen(getcode),0,0,(GAsyncReadyCallback)smsserver_send_getX_ready,user_data);
			}
			else{
				//verify code
				gchar * verify = g_strdup_printf("%s&code=%s\n\n",verify_code,user_data->code);
				g_output_stream_write_async(g_io_stream_get_output_stream(G_IO_STREAM(user_data->connec)),
						verify,strlen(verify),0,0,(GAsyncReadyCallback)smsserver_send_getX_ready,user_data);
			}
		break;
		case 401: //TODO:发送MD5密码
			seed = strstr(user_data->readbuffer,"\n");
			if(seed)
			{
				char seedcode[12]={0};
				seed ++;
				if(sscanf(seed,"SEED%*[ :]%11[0-9a-fA-F]\n\n",seedcode)==1)
				{
					//开始加密密码
					compass = g_strdup_printf("%s%s",passwd,seedcode);

					Computehash((guchar*)compass,strlen(compass),digest);
					g_free(compass);

					int len = snprintf(user_data->readbuffer,sizeof(user_data->readbuffer),"PASSWD %02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\n\n",
							digest[0],digest[1],digest[2],digest[3],digest[4],digest[5],digest[6],digest[7],
							digest[8],digest[9],digest[10],digest[11],digest[12],digest[13],digest[14],digest[15]);

					g_output_stream_write_async(g_io_stream_get_output_stream(G_IO_STREAM(user_data->connec)),
						user_data->readbuffer,len,0,0,(GAsyncReadyCallback)smsserver_send_ready,user_data);
					break;
				}
			}
		default:
			CALL_USER_CB(user_data,0);
			g_object_unref(user_data->connec);
			g_slice_free(smscbdata,user_data);
			break;
	}
}

static void smsserver_send_ready(GOutputStream *source_object,GAsyncResult *res, smscbdata* user_data)
{
	gssize ret = g_output_stream_write_finish(source_object,res,0);

	if(ret <=0)
	{
		CALL_USER_CB(user_data,0);
		g_object_unref(user_data->connec);
		g_slice_free(smscbdata,user_data);
		return ;
	}
	memset(user_data->readbuffer,0,sizeof(user_data->readbuffer));

	g_input_stream_read_async(g_io_stream_get_input_stream(G_IO_STREAM(user_data->connec)),
			user_data->readbuffer,sizeof(user_data->readbuffer),0,0,(GAsyncReadyCallback)smsserver_recv_user_login_ready,user_data);

}

static void smsserver_connected(GSocketClient *source_object,GAsyncResult *res, smscbdata* user_data)
{
	GSocketConnection * connec =  g_socket_client_connect_to_host_finish(source_object,res,NULL);
	if(!connec) // NOT connecteable
	{
		CALL_USER_CB(user_data,0);
		g_slice_free(smscbdata,user_data);
		return ;
	}
	user_data->connec = connec;
	g_object_ref(connec);

//	int use_auth = 0;
//	if(use_auth){
	//发送登录口令
	g_output_stream_write_async(g_io_stream_get_output_stream(G_IO_STREAM(connec)),user_login,strlen(user_login),0,0,(GAsyncReadyCallback)smsserver_send_ready,user_data);
//	}else{
		//或则直接使用?
//		g_output_stream_write_async(g_io_stream_get_output_stream(G_IO_STREAM(user_data->connec)),getcode,strlen(getcode),0,0,(GAsyncReadyCallback)smsserver_send_getcode_ready,user_data);
//	}
}

void smsserver_getcode(smsserver_readycallback usercb,SoupServer *server,SoupMessage * msg,const char *path,GHashTable *query, SoupClientContext *client,gpointer user_data)
{
	smscbdata * data = g_slice_new0(smscbdata);

	data->cb = usercb;
	data->data  = user_data;
	data->server = server;
	data->msg = msg;
	data->client = client;
	data->path = path;
	data->query = query;
	//开始连接到服务器
	g_socket_client_connect_to_host_async(connector,smshost,smsport,0,(GAsyncReadyCallback)smsserver_connected,data);
}

void smsserver_verifycode(smsserver_readycallback usercb,const char * code,SoupServer *server,SoupMessage * msg,const char *path,GHashTable *query, SoupClientContext *client,gpointer user_data)
{
	smscbdata * data = g_slice_new0(smscbdata);

	data->cb = usercb;
	data->data  = user_data;
	data->server = server;
	data->msg = msg;
	data->client = client;
	data->path = path;
	data->query = query;
	strncpy(data->code,code,29);
	//开始连接到服务器
	g_socket_client_connect_to_host_async(connector,smshost,smsport,0,(GAsyncReadyCallback)smsserver_connected,data);
}



