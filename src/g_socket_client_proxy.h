/*
 * g_socket_client.h - 代理支持的连接器
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

#ifndef G_SOCKET_CLIENT_PROXY_H_
#define G_SOCKET_CLIENT_PROXY_H_

typedef enum _ProxyType{
	G_SOCKET_CLIENT_PROXY_NONE, // direct call
	G_SOCKET_CLIENT_PROXY_HTTP,
	G_SOCKET_CLIENT_PROXY_SOCKS4,
	G_SOCKET_CLIENT_PROXY_SOCKS5,
	_G_SOCKET_CLIENT_PROXY_UNUSED
}GSocketClientProxyType;


typedef struct _GSocketClientProxy GSocketClientProxy;

typedef struct _GSocketClientProxyClass GSocketClientProxyClass;

#define G_TYPE_SOCKET_CLIENT_PROXY	g_socket_client_proxy_get_type()
#define G_SOCKET_CLIENT_PROXY(object)            (G_TYPE_CHECK_INSTANCE_CAST ((object), G_TYPE_SOCKET_CLIENT_PROXY, GSocketClientProxy))


GType g_socket_client_proxy_get_type();

GSocketClientProxy *g_socket_client_proxy_new(GSocketClientProxyType type);


void g_socket_proxy_connect_to_host_async(GSocketClientProxy* client,
		const gchar *host_and_port, guint16 default_port, gchar * proxy_address_andport,
		GCancellable *cancellable, GAsyncReadyCallback callback,gpointer user_data);

GSocketConnection* g_socket_proxy_connect_to_host_finish(GSocketClientProxy* client,GAsyncResult * res,GError ** err);

#endif /* G_SOCKET_CLIENT_PROXY_H_ */
