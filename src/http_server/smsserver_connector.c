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
#include <gnutls/gnutls.h>
#include "global.h"
#include "smsserver_connector.h"
#include "socket/g_socket_client_proxy.h"

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


#define smsport 25720

static GSocketClient	* connector;
static gchar			  smshost[180];
static gchar * 			user_login;
static gboolean			isonline ;
static gchar * 			getcode;
static gchar * 			getsmsc; //获得 smsd 的地址
static gchar * 			verify_code;
static gchar				passwd[160] = "123456";
static gchar * smssched;

static void smsserver_loop_connected(GSocketClient *source_object,GAsyncResult *res, smscbdata* user_data);

typedef struct _loop_iter_struct{
	gssize size;
	GMainLoop * loop;
}loop_iter_struct;

static  ssize_t tls_pull_from_in_stream(gnutls_transport_ptr_t ins, void * buf,size_t s)
{
	loop_iter_struct dt;

	dt.loop = g_main_loop_new(g_main_context_get_thread_default(),0);

	void cb(GObject * source, GAsyncResult * res , gpointer data)
	{
		((loop_iter_struct*)data)->size = g_input_stream_read_finish(G_INPUT_STREAM(source),res,0);
		g_main_loop_quit(((loop_iter_struct*)data)->loop);
	}

	g_input_stream_read_async(ins,buf,s,0,0,cb,&dt);

	g_main_loop_run(dt.loop);
	g_main_loop_unref(dt.loop);
	return dt.size;
}

static  ssize_t tls_push_to_out_stream(gnutls_transport_ptr_t outs,const void *buf,size_t s)
{
	loop_iter_struct dt;

	dt.loop = g_main_loop_new(g_main_context_get_thread_default(),0);

	void cb(GObject * source, GAsyncResult * res , gpointer data)
	{
		((loop_iter_struct*)data)->size = g_output_stream_write_finish(G_OUTPUT_STREAM(source),res,0);
		g_main_loop_quit(((loop_iter_struct*)data)->loop);
	}

	g_output_stream_write_async(outs,buf,s,0,0,cb,&dt);

	g_main_loop_run(dt.loop);
	g_main_loop_unref(dt.loop);
	return dt.size;

}

static gboolean lets_loop_connect(gpointer user_data)
{
	g_socket_client_connect_to_host_async(connector,smshost,smsport,0,(GAsyncReadyCallback)smsserver_loop_connected,NULL);
	return FALSE;
}


static gboolean connect_sched(gpointer pointer);

static void smssched_connected(GSocketClient *source_object,GAsyncResult *res, smscbdata* user_data)
{
	GInputStream * ins;
	GOutputStream* outs;

	int tlsret;

	GSocketConnection * connec =  g_socket_client_connect_to_host_finish(source_object,res,NULL);

	if(!connec)
	{
#ifdef DEBUG
		g_warning("短信调度中心不在线，滚");
#endif
		g_timeout_add_seconds(5,connect_sched,0);
		return ;
	}

	// get input stream
	ins = g_io_stream_get_input_stream(G_IO_STREAM(connec));
	// get output stream
	outs = g_io_stream_get_output_stream(G_IO_STREAM(connec));

	//开始 TLS 连接
	gnutls_anon_client_credentials_t anoncred;
	gnutls_session_t session;

	tlsret = gnutls_init(&session,GNUTLS_CLIENT);

	tlsret = gnutls_anon_allocate_client_credentials(&anoncred);

	gnutls_priority_set_direct (session, "NORMAL:+ANON-DH", NULL);

	tlsret = gnutls_credentials_set(session,GNUTLS_CRD_ANON,anoncred);

	gnutls_transport_set_ptr(session,connec);
	gnutls_transport_set_ptr2(session,ins,outs);

	gnutls_transport_set_pull_function(session,tls_pull_from_in_stream);
	gnutls_transport_set_push_function(session,tls_push_to_out_stream);

	if (gnutls_handshake(session))
	{

#ifdef DEBUG
		g_warning("短信调度TLS握手错误");
#endif
		g_timeout_add_seconds(5,connect_sched,0);
		goto disconnect;
	}

	tlsret = gnutls_record_send(session,getsmsc,strlen(getsmsc));

	if(tlsret != strlen(getsmsc))
	{
		g_timeout_add_seconds(5,connect_sched,0);
		goto disconnect;
	}

	char data[512];
	gnutls_record_recv(session,data,sizeof(data));
	data[511] = 0;

	gint status;

	gboolean prase_data(gint * status, char smshost[], char passwd[])
	{
		return sscanf(data,"%d%*[^\n]%*[^:]%*[ :]%[^\n]\npasswd:%[^\n]\n",status,smshost,passwd)==3;
	}

	if(!prase_data(&status,smshost,passwd))
	{
		g_timeout_add_seconds(5,connect_sched,0);
		goto disconnect;
	}

	if(status != 200)
	{
		g_timeout_add_seconds(5,connect_sched,0);
		goto disconnect;
	}

	g_message("使用短信中心:%s",smshost);

 	lets_loop_connect(connector);
 	//开始不停的连接吧，哈哈
// 	g_timeout_add_seconds(5,lets_loop_connect,connector);
disconnect:
	gnutls_bye(session,GNUTLS_SHUT_RDWR);
	gnutls_deinit(session);
	gnutls_anon_free_client_credentials(anoncred);

	if(connec)
		g_object_unref(connec);
}

static void smsserver_loop_connected(GSocketClient *source_object,GAsyncResult *res, smscbdata* user_data)
{
	GSocketConnection * connec =  g_socket_client_connect_to_host_finish(source_object,res,NULL);

	isonline = (connec!=NULL);

	if (connec)
	{
		g_object_unref(connec);
		g_timeout_add_seconds(5,lets_loop_connect,connector);
	}
	else
	{
		//重新连接到调度中心
		g_timeout_add_seconds(5,connect_sched,0);
	}

#ifdef DEBUG
	g_debug("短信服务器 %s 目前%s",smshost,isonline?"在线":"不在线");
#endif

}

static gboolean connect_sched(gpointer pointer)
{
	//开始连接到调度服务器
	g_socket_client_connect_to_host_async(connector,smssched,25800,0,(GAsyncReadyCallback)smssched_connected,0);
	return FALSE;
}

void smsserver_pinger_start()
{
	gnutls_global_init();
	//获得 smshost
	//开始空白连接，哦哈哈
	gchar * hid=g_strstrip(g_key_file_get_string(gkeyfile,"kpolice","HotelID",0));
	user_login = g_strdup_printf("USER %s\n\n",hid);
	getcode = g_strdup_printf("GET /gencode?ID=%s\n\n",hid);
	getsmsc = g_strdup_printf("GET /smsc?ID=%s\n\n",hid);
	verify_code = g_strdup_printf("GET /verifycode?ID=%s",hid);
	g_free(hid);

	//调度中心地址
	smssched = g_strstrip(g_key_file_get_string(gkeyfile,"sms","smssched",0));

	connector = g_socket_client_new();
	//只给你 10s 的时间完成连接，否则，嘿嘿
	g_socket_client_set_timeout(connector,10);

	g_timeout_add_seconds(0,connect_sched,0);
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
	gchar * compass;
	GChecksum * gmd5;
	gssize ret;

	ret = g_input_stream_read_finish(source_object,res,0);

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
		case 401:
			seed = strstr(user_data->readbuffer,"\n");
			if(seed)
			{
				char seedcode[12]={0};
				seed ++;
				if(sscanf(seed,"SEED%*[ :]%11[0-9a-fA-F]\n\n",seedcode)==1)
				{
					//开始加密密码
					compass = g_strdup_printf("%s%s",passwd,seedcode);

					gmd5 = g_checksum_new(G_CHECKSUM_MD5);

					g_checksum_update(gmd5,(guchar*)compass,strlen(compass));

					g_free(compass);

					int len = snprintf(user_data->readbuffer,sizeof(user_data->readbuffer),"PASSWD %s\n\n",g_checksum_get_string(gmd5));

					g_checksum_free(gmd5);

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

//	int use_auth = 0;
//	if(use_auth){
	//发送登录口令
	g_output_stream_write_async(g_io_stream_get_output_stream(G_IO_STREAM(user_data->connec)),user_login,strlen(user_login),0,0,(GAsyncReadyCallback)smsserver_send_ready,user_data);
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



