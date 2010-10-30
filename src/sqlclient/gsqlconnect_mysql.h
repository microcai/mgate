/*
 * gsqlconnect_mysql.h
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
#define G_SQL_CONNECT_MYSQL(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), G_TYPE_SQL_CONNNECT_MYSQL, GSQLConnectMysql))
#define G_SQL_CONNECT_MYSQL_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), G_TYPE_SQL_CONNNECT_MYSQL, GSQLConnectMysqlClass))
#define IS_G_SQL_CONNECT_MYSQL(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), G_TYPE_SQL_CONNNECT_MYSQL))
#define IS_G_SQL_CONNECT_MYSQL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), G_TYPE_SQL_CONNNECT_MYSQL))
#define G_SQL_CONNECT_MYSQL_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), G_TYPE_SQL_CONNNECT_MYSQL, GSQLConnectMysqlClass))



GType g_sql_connect_mysql_get_type() G_GNUC_CONST;

#ifndef __HAVE_MYSQL
GType g_sql_connect_mysql_get_type(){
	return 0;
}
#endif

#endif /* GSQLCONNECT_MYSQL_H_ */
