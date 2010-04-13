/*
 * gsqlconnect_mysql.h
 *
 *  Created on: 2010-4-13
 *      Author: cai
 */

#ifndef GSQLCONNECT_MYSQL_H_
#define GSQLCONNECT_MYSQL_H_

#if defined HAVE_MYSQL_MYSQL_H || defined HAVE_MYSQL_H
#include <mysql/mysql.h>
#define HAVE_MYSQL 1
#else
typedef struct _MYSQL{ char pad[964]; }MYSQL;
#endif

#include <glib.h>
#include <glib-object.h>
#include "gsqlconnect.h"

typedef struct _GSQLConnectMysql{
	GSQLConnect	parent;
	MYSQL		mysql[1];
	gchar	*	porperty_string[8];
}GSQLConnectMysql;

typedef struct _GSQLConnectMysqlClass{
	GSQLConnectClass parent_class;
}GSQLConnectMysqlClass;

#define G_TYPE_SQL_CONNNECT_MYSQL	g_sql_connect_mysql_get_type()
#define G_SQL_CONNECT_MYSQL(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), G_TYPE_SQL_CONNNECT, GSQLConnectMysql))
#define G_SQL_CONNECT_MYSQL_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), G_TYPE_SQL_CONNNECT, GSQLConnectMysqlClass))
#define IS_G_SQL_CONNECT_MYSQL(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), G_TYPE_SQL_CONNNECT))
#define IS_G_SQL_CONNECT_MYSQL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), G_TYPE_SQL_CONNNECT))
#define G_SQL_CONNECT_MYSQL_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), G_TYPE_SQL_CONNNECT, GSQLConnectMysqlClass))



GType g_sql_connect_mysql_get_type() G_GNUC_CONST;
#endif /* GSQLCONNECT_MYSQL_H_ */
