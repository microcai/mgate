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
	gboolean	(*check_config)(GSQLConnect*);
	gboolean	(*connect)(GSQLConnect*);
	gboolean	(*run_query)(GSQLConnect*,const char * sql_stmt,gsize len /* -1 for nul-terminated string*/);
	GList*		(*get_resust)(GSQLConnect*,gpointer host,gpointer user,gpointer passwd);
}GSQLConnectClass;

#define G_TYPE_SQL_CONNNECT	g_sql_connect_get_type()
#define G_SQL_CONNECT(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), G_TYPE_SQL_CONNNECT, GSQLConnect))
#define G_SQL_CONNECT_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), G_TYPE_SQL_CONNNECT, GSQLConnectClass))
#define IS_G_SQL_CONNECT(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), G_TYPE_SQL_CONNNECT))
#define IS_G_SQL_CONNECT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), G_TYPE_SQL_CONNNECT))
#define G_SQL_CONNECT_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), G_TYPE_SQL_CONNNECT, GSQLConnectClass))

GType g_sql_connect_get_type() G_GNUC_CONST;

gboolean g_sql_connect_check_config(GSQLConnect*);

#endif /* GSQLCONNECT_H_ */
