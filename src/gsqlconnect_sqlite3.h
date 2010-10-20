/*
 * gsqlconnect_sqlite.h
 *
 *  Created on: 2010-4-13
 *      Author: cai
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
