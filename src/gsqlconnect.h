/*
 * gsqlconnect.h
 *
 *  Created on: 2010-4-13
 *      Author: cai
 */

#ifndef GSQLCONNECT_H_
#define GSQLCONNECT_H_

#include <glib.h>
#include <glib-object.h>


typedef struct _GSQLConnect{
	GObject	parent;
}GSQLConnect;

typedef struct _GSQLConnectClass{
	GObjectClass parent_class;
	gboolean	(*connect)(GSQLConnect*);
	gboolean	(*run_query)(GSQLConnect*,const char * sql_stmt,gsize len /* -1 for nul-terminated string*/);
	GList*		(*get_resust)(GSQLConnect*,gpointer host,gpointer user,gpointer passwd);
}GSQLConnectClass;

#define G_TYPE_SQL_CONNNECT	g_sql_connect_get_type()



GType g_sql_connect_get_type() G_GNUC_CONST;


#endif /* GSQLCONNECT_H_ */
