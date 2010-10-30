/*
 * gsqlconnect.h
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

#ifndef GSQLCONNECT_H_
#define GSQLCONNECT_H_

#include <glib.h>
#include <glib-object.h>
#include "gsqlresult.h"

typedef struct _GSQLConnect{
	GObject		parent;
	GObject *lastresult;
}GSQLConnect;

typedef struct _GSQLConnectClass{
	GObjectClass parent_class;
	gboolean	(*check_config)(GSQLConnect*);
	gboolean	(*connect)(GSQLConnect*,GError **);
	gboolean	(*run_query)(GSQLConnect*,const char * sql_stmt,gsize len /* -1 for nul-terminated string*/);
	gboolean	(*ping)(GSQLConnect*,GError **);
}GSQLConnectClass;

#define G_TYPE_SQL_CONNNECT	g_sql_connect_get_type()
#define G_SQL_CONNECT(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), G_TYPE_SQL_CONNNECT, GSQLConnect))
#define G_SQL_CONNECT_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), G_TYPE_SQL_CONNNECT, GSQLConnectClass))
#define IS_G_SQL_CONNECT(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), G_TYPE_SQL_CONNNECT))
#define IS_G_SQL_CONNECT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), G_TYPE_SQL_CONNNECT))
#define G_SQL_CONNECT_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), G_TYPE_SQL_CONNNECT, GSQLConnectClass))

GType g_sql_connect_get_type() G_GNUC_CONST;

gboolean g_sql_connect_check_config(GSQLConnect*obj);

gboolean g_sql_connect_real_connect(GSQLConnect* obj,
									GError ** err);

gboolean g_sql_connect_run_query(GSQLConnect * obj,
								 const gchar * sqlstatement,
								 gsize size);

GSQLResult* g_sql_connect_use_result(GSQLConnect * obj);

gboolean g_sql_connect_ping(GSQLConnect * obj,GError ** err);

extern void (*g_sql_connect_thread_init)();

extern void (*g_sql_connect_thread_end)();


GType	sqlconnect_get_backend();
#endif /* GSQLCONNECT_H_ */
