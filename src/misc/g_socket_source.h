/*
 * g_socket_source.h
 *
 *  Created on: 2010-10-27
 *      Author: cai
 */

#ifndef G_SOCKET_SOURCE_H_
#define G_SOCKET_SOURCE_H_

#include <gio/gio.h>

typedef gboolean (*GSockIOFunc) (GSocket   *source,
			     GIOCondition  condition,
			     gpointer      data);


guint g_socket_add_watch(GSocket *channel, GIOCondition condition,
		GSockIOFunc func, gpointer user_data);

#endif /* G_SOCKET_SOURCE_H_ */
