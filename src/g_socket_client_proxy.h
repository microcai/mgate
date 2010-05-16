/*
 * g_socket_client.h
 *
 *  Created on: 2010-5-17
 *      Author: cai
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
