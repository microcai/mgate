/*
 * g_socket_source.h
 *
 *  Created on: 2010-11-3
 *      Author: cai
 */

#ifndef G_SOCKET_SOURCE_H_
#define G_SOCKET_SOURCE_H_

#include <gio/gio.h>


guint g_socket_add_watch(GSocket * sock, GIOCondition con, GSocketSourceFunc func,gpointer user_data);

void g_socket_connect_to_host_async(const gchar * host, guint16 port,GAsyncReadyCallback cb, gpointer user_data);
GSocketConnection * g_socket_connect_to_host_finish(GSocket * sock,GAsyncResult * res);


#endif /* G_SOCKET_SOURCE_H_ */
