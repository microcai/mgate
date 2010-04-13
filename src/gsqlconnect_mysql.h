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
}GSQLConnectMysql;

typedef struct _GSQLConnectMysqlClass{
	GSQLConnectClass parent_class;
}GSQLConnectMysqlClass;

#define G_TYPE_SQL_CONNNECT_MYSQL	g_sql_connect_mysql_get_type()



GType g_sql_connect_mysql_get_type() G_GNUC_CONST;
#endif /* GSQLCONNECT_MYSQL_H_ */
