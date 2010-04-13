/*
 * gsqlconnect_mysql.h
 *
 *  Created on: 2010-4-13
 *      Author: cai
 */

#ifndef GSQLCONNECT_MYSQL_H_
#define GSQLCONNECT_MYSQL_H_

#include <glib.h>
#include <glib-object.h>
#include "gsqlconnect.h"


typedef struct _GSQLConnectMysql{
	GSQLConnect	parent;
}GSQLConnectMysql;

typedef struct _GSQLConnectMysqlClass{
	GSQLConnectClass parent_class;
}GSQLConnectMysqlClass;

#define G_TYPE_SQL_CONNNECT_MYSQL	g_sql_connect_mysql_get_type()



GType g_sql_connect_mysql_get_type() G_GNUC_CONST;
#endif /* GSQLCONNECT_MYSQL_H_ */
