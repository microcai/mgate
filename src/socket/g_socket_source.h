/*
 * g_socket_source.h
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

#ifndef G_SOCKET_SOURCE_H_
#define G_SOCKET_SOURCE_H_

#include <gio/gio.h>


guint g_socket_add_watch(GSocket * sock, GIOCondition con, GSocketSourceFunc func,gpointer user_data);

void g_socket_connect_to_host_async(const gchar * host, guint16 port,GAsyncReadyCallback cb, gpointer user_data);
GSocketConnection * g_socket_connect_to_host_finish(GSocket * sock,GAsyncResult * res);


#endif /* G_SOCKET_SOURCE_H_ */
