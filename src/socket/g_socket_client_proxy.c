/*
 * g_socket_client.c -- 代理支持的连接器
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
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <sys/syslog.h>

#include <signal.h>
#include <errno.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <memory.h>
#include <stdlib.h>
#include <stdio.h>
#include <glib.h>
#include <gio/gio.h>
#include <glib/gi18n.h>

#include "g_socket_client_proxy.h"

struct _GSocketClientProxy{
	GSocketClient parent;
	GSocketClientProxyType proxy_type;
};

struct _GSocketClientProxyClass{
	GSocketClientClass parent_class;
};

typedef struct _AsyncProxyTag{
	GSocketConnection * connection;
	gchar*		  dest_host;
	gint		  dest_host_port;
	gboolean	  connected;
}AsyncProxyTag;

static void async_proxy_tag_free(AsyncProxyTag * tag)
{
	g_free(tag->dest_host);
	g_free(tag);
}

static void g_socket_client_proxy_class_init(GSocketClientProxyClass*);
static void g_socket_client_proxy_init(GSocketClientProxy*);
static void g_socket_client_proxy_finalize(GSocketClientProxy*);
static void g_socket_client_proxy_dispose(GSocketClientProxy*);
static void g_socket_client_proxy_set_property(GObject *object,
		guint property_id, const GValue *value, GParamSpec *pspec);
static void g_socket_client_proxy_get_property(GObject *object,
		guint property_id, GValue *value, GParamSpec *pspec);
static void g_socket_add_watch(GSocket * sock,GIOCondition con, gpointer callback, gpointer user_data, gpointer des_notify);


G_DEFINE_TYPE(GSocketClientProxy,g_socket_client_proxy,G_TYPE_SOCKET_CLIENT);

static void g_socket_client_proxy_class_init(GSocketClientProxyClass*klass)
{
	GParamSpec * spec = g_param_spec_ulong("proxy-type","proxy-type","proxy-type",G_SOCKET_CLIENT_PROXY_NONE,_G_SOCKET_CLIENT_PROXY_UNUSED-1,G_SOCKET_CLIENT_PROXY_NONE,G_PARAM_CONSTRUCT_ONLY|G_PARAM_READWRITE|G_PARAM_STATIC_NAME);

	G_OBJECT_CLASS(klass)->set_property = g_socket_client_proxy_set_property;
	G_OBJECT_CLASS(klass)->get_property = g_socket_client_proxy_get_property;

	g_object_class_install_property(G_OBJECT_CLASS(klass),1,spec);
}

static void g_socket_client_proxy_init(GSocketClientProxy* client)
{

}

void g_socket_client_proxy_get_property(GObject *object,guint property_id, GValue *value, GParamSpec *pspec)
{
	g_value_set_ulong(value,G_SOCKET_CLIENT_PROXY(object)->proxy_type);
}

void g_socket_client_proxy_set_property(GObject *object,guint property_id, const GValue *value, GParamSpec *pspec)
{
	G_SOCKET_CLIENT_PROXY(object)->proxy_type = g_value_get_ulong(value);
}

GSocketConnection* g_socket_proxy_connect_to_host_finish(GSocketClientProxy* client,GAsyncResult * res,GError ** err)
{
	if(client->proxy_type==G_SOCKET_CLIENT_PROXY_NONE)
	{
		return g_socket_client_connect_to_host_finish(G_SOCKET_CLIENT(client),res,err);
	}

	AsyncProxyTag * tag = g_simple_async_result_get_op_res_gpointer(G_SIMPLE_ASYNC_RESULT(res));

	gboolean connected = tag->connected;
	GSocketConnection * con = tag->connection ;

	g_object_unref(client);
	g_object_unref(res);

	return connected?con:NULL;
}

static gboolean proxy_say_ok(GSocket * sock,GIOCondition condition,GSimpleAsyncResult * res)
{
	GSocketClientProxyType type;
	GSocketClientProxy * client = (gpointer) g_async_result_get_source_object(G_ASYNC_RESULT(res));
	type = client->proxy_type;
	g_object_unref(client);

	if(condition == G_IO_IN)
	{
		gchar buffer[64];
		gint retcode;

		gssize recved = g_socket_receive(sock, buffer, sizeof(buffer), 0,0);

		switch (type) {
			case G_SOCKET_CLIENT_PROXY_HTTP:
				if(recved > 16 && sscanf(buffer, "HTTP/%*[^ ]%d ",&retcode) == 1 && retcode == 200)
				{
					((AsyncProxyTag*)g_simple_async_result_get_op_res_gpointer(res))->connected = TRUE;
				}
				break;
			case G_SOCKET_CLIENT_PROXY_SOCKS5:
				if(buffer[0]==5 && buffer[1]==0)
				{
					((AsyncProxyTag*)g_simple_async_result_get_op_res_gpointer(res))->connected = TRUE;
				}
				break;
		}
	}
	g_simple_async_result_complete(res);
	return 0;
}

static gboolean proxy_send_connect(GSocket * sock,GIOCondition condition,GSimpleAsyncResult * res)
{
	gchar * connect ;
	gssize sended;
	GSocketClientProxyType type;
	GSocketClientProxy * client = (gpointer) g_async_result_get_source_object(G_ASYNC_RESULT(res));
	type = client->proxy_type;
	g_object_unref(client);

	gchar buffer[80];
	gint len;

	if(condition == G_IO_OUT)
	{
		AsyncProxyTag * tag = g_simple_async_result_get_op_res_gpointer(res);

		switch (type)
		{
			//构造 CONNECT
		case G_SOCKET_CLIENT_PROXY_HTTP:
			connect = g_strdup_printf(
					"CONNECT %s:%d HTTP/1.1\r\n\r\n", tag->dest_host,tag->dest_host_port);
			sended = g_socket_send(sock, connect, strlen(connect),0, 0);
			if (sended == (strlen(connect)))
			{
				//现在，我们要读取
				g_socket_add_watch(sock, G_IO_IN, proxy_say_ok, res, 0);
				return 0;
			}
			break;
		case G_SOCKET_CLIENT_PROXY_SOCKS5:

			buffer[0] = 5;
			buffer[1] = 1;
			buffer[2] = 0;
			buffer[3] = 3;

			len = strlen(tag->dest_host);

			buffer[4] = len;

			strcpy(&buffer[5],tag->dest_host);

			*(gushort*)(buffer + 5 + len ) = htons(tag->dest_host_port);

			sended = g_socket_send(sock,buffer,len + 5 +2,0,0);

			if(sended >0)
				g_socket_add_watch(sock, G_IO_IN, proxy_say_ok, res, 0);
			return 0;
		}
	}
	g_simple_async_result_complete(res);
	return 0;
}

static gboolean proxy_check_valid(GSocket * sock,GIOCondition condition,GSimpleAsyncResult * res)
{
	char buf[2];

	if(condition==G_IO_IN)
	{
		if (g_socket_receive(sock, buf, 2, 0, 0) == 2)
		{
			if (buf[0] == 5 && buf[1] == 0)
			{
				g_socket_add_watch(sock, G_IO_OUT, proxy_send_connect, res, 0);
				return 0;
			}
		}

	}
	g_simple_async_result_complete(res);
	return 0;
}

static gboolean proxy_send_hello(GSocket * sock,GIOCondition condition,GSimpleAsyncResult * res)
{
	gssize sended;

	GSocketClientProxyType type;
	GSocketClientProxy * client = (gpointer) g_async_result_get_source_object(G_ASYNC_RESULT(res));
	type = client->proxy_type;
	g_object_unref(client);


	if(condition==G_IO_OUT)
	{
		AsyncProxyTag * tag = g_simple_async_result_get_op_res_gpointer(res);

		switch (type)
		{
			//构造 CONNECT
			case G_SOCKET_CLIENT_PROXY_HTTP:
					//现在，我们要发送 CONNECT 命令
				g_socket_add_watch(sock,G_IO_OUT,proxy_send_connect,res,0);
				return 0;
			case G_SOCKET_CLIENT_PROXY_SOCKS5:
				sended = g_socket_send(sock,"\005\001\000",3,0,0);
				if(sended ==3)
				{
					g_socket_add_watch(sock,G_IO_IN,proxy_check_valid,res,0);
					return 0;
				}
				break;
		}
	}
	g_simple_async_result_complete(res);
	return 0;
}

static void g_socket_proxy_connected(GSocketClientProxy* client,GAsyncResult * res, GSimpleAsyncResult * simple_res)
{

	GSocketConnection * connection = g_socket_client_connect_to_host_finish(G_SOCKET_CLIENT(client),res,0);

	if(!connection){
		return g_simple_async_result_complete(simple_res);
	}
	AsyncProxyTag * tag = g_simple_async_result_get_op_res_gpointer(simple_res);

	tag->connection = connection;

	//开始和代理服务器交互
	GSocket * sock = g_socket_connection_get_socket(connection);

	//等待代理服务器能发送
	g_socket_add_watch(sock,G_IO_OUT,proxy_send_hello,simple_res,0);
}

void g_socket_proxy_connect_to_host_async(GSocketClientProxy* client,
		const gchar *host_and_port, guint16 default_port, gchar * proxy_address_andport,
		GCancellable *cancellable, GAsyncReadyCallback callback,gpointer user_data)
{
	if(client->proxy_type==G_SOCKET_CLIENT_PROXY_NONE)
	{
		return g_socket_client_connect_to_host_async(G_SOCKET_CLIENT(client),host_and_port,default_port,cancellable,callback,user_data);
	}

	g_object_ref(client);
	GSimpleAsyncResult * res = g_simple_async_result_new(G_OBJECT(client),callback,user_data,g_socket_proxy_connect_to_host_async);

	AsyncProxyTag * tag = g_new0(AsyncProxyTag,1);
	tag->dest_host = g_strdup(host_and_port);
	tag->dest_host = strtok(tag->dest_host,":");
	char * port = strtok(NULL,"");
	if(port)
		tag->dest_host_port = atoi(port);
	else
		tag->dest_host_port = default_port;

	g_simple_async_result_set_op_res_gpointer(res,tag,(GDestroyNotify)async_proxy_tag_free);
	g_socket_client_connect_to_host_async(G_SOCKET_CLIENT(client),proxy_address_andport,0,cancellable,(GAsyncReadyCallback)g_socket_proxy_connected,res);
}

GSocketClientProxy *g_socket_client_proxy_new(GSocketClientProxyType type)
{
	return g_object_new(G_TYPE_SOCKET_CLIENT_PROXY,"proxy-type",type,NULL);
}

void g_socket_add_watch(GSocket * sock,GIOCondition con, gpointer callback, gpointer user_data, gpointer des_notify)
{
	GSource * source = g_socket_create_source(sock,con,0);
	g_source_set_callback(source,callback,user_data,des_notify);
	g_source_attach(source,g_main_context_get_thread_default());
	g_source_unref(source);
}

