/*
 * gsqlconnect_sqlite.h
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

#ifndef GSQLCONNECT_SQLITE_H_
#define GSQLCONNECT_SQLITE_H_

#ifdef WITH_SQLITE3
#include <sqlite3.h>

#include <glib.h>
#include <glib-object.h>
#include "gsqlconnect.h"

typedef struct _GSQLConnectSqlite3{
	GSQLConnect	parent;
	sqlite3	*	sqlite;
	gchar	*	file;
}GSQLConnectSqlite3;

typedef struct _GSQLConnectSqlite3Class{
	GSQLConnectClass parent_class;
}GSQLConnectSqlite3Class;

#define G_TYPE_SQL_CONNNECT_SQLITE	g_sql_connect_sqlite3_get_type()
#define G_SQL_CONNECT_SQLITE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), G_TYPE_SQL_CONNNECT_SQLITE, GSQLConnectSqlite3))
#define G_SQL_CONNECT_SQLITE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), G_TYPE_SQL_CONNNECT_SQLITE, GSQLConnectSqlite3Class))
#define IS_G_SQL_CONNECT_SQLITE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), G_TYPE_SQL_CONNNECT_SQLITE))
#define IS_G_SQL_CONNECT_SQLITE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), G_TYPE_SQL_CONNNECT_SQLITE))
#define G_SQL_CONNECT_SQLITE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), G_TYPE_SQL_CONNNECT_SQLITE, GSQLConnectSqlite3Class))

#endif

GType g_sql_connect_sqlite3_get_type() G_GNUC_CONST;

#ifndef WITH_SQLITE3
GType g_sql_connect_sqlite3_get_type(){
	return 0;
}
#endif

#endif /* GSQLCONNECT_SQLITE_H_ */
