/*
 * g_socket_io_sourrce.c
 *
 *  Created on: 2010-11-3
 *      Author: cai
 */
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "g_socket_source.h"

typedef struct Gprivate
{
	GSocket * sock;
	gpointer	 data;
	GAsyncReadyCallback	 cb;
	gushort	port;
} Gprivate;

guint g_socket_add_watch(GSocket * sock, GIOCondition con, GSocketSourceFunc func,gpointer user_data)
{
//	g_io_add_watch()
	GSource * source = g_socket_create_source(sock,con,0);

	g_source_set_callback(source,(GSourceFunc)func,user_data,0);

	return g_source_attach(source,g_main_context_get_thread_default());
}

static gboolean wapper(GSocket *socket, GIOCondition condition,Gprivate * ptr)
{
	ptr->cb((GObject*)socket,(GAsyncResult*)ptr,ptr->data);
	return FALSE;
}

static void dns_ready_cb(GResolver * dns,GAsyncResult * res, Gprivate * ptr)
{
	GList * list = g_resolver_lookup_by_name_finish(dns,res,0);

	if(list)
	{
		GSocketAddress * add = g_inet_socket_address_new((g_list_first(list)->data),ptr->port);
		g_socket_connect(ptr->sock,add,0,0);
		g_socket_add_watch(ptr->sock,G_IO_OUT,(GSocketSourceFunc)wapper,ptr);
		g_object_unref(ptr->sock);
		g_object_unref(add);
		g_resolver_free_addresses(list);
	}
	else
	{
		g_object_unref(ptr->sock);
		ptr->cb(NULL,NULL,ptr->data);
		g_slice_free(Gprivate,ptr);
	}
}

void g_socket_connect_to_host_async(const gchar * host, guint16 port,GAsyncReadyCallback cb, gpointer user_data)
{
	GSocket * sock;
	Gprivate * ptr = g_slice_new0(Gprivate);

	int fd = socket(AF_INET,SOCK_STREAM|SOCK_CLOEXEC|SOCK_NONBLOCK,0);

	sock = g_socket_new_from_fd(fd,0);
	g_socket_set_blocking(sock,FALSE);

	ptr->sock = sock;
	ptr->data = user_data;
	ptr->cb = cb;
	ptr->port = port;

	GResolver * dns =g_resolver_get_default();
	g_resolver_lookup_by_name_async(dns,host,0,(GAsyncReadyCallback)dns_ready_cb,ptr);
	g_object_unref(dns);
}

GSocketConnection * g_socket_connect_to_host_finish(GSocket * sock,GAsyncResult * res)
{
	GSocketConnection * connection = NULL;

	if(res && g_socket_check_connect_result(sock,0))
	{
		//构造 GSocketConnection
		connection=g_socket_connection_factory_create_connection(g_object_ref(sock));
	}
	if(res)
		g_slice_free(Gprivate,(Gprivate*)res);
	if(sock)
		g_object_unref(sock);
	return connection;
}
