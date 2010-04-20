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

gboolean g_sql_connect_check_config(GSQLConnect*);
gboolean g_sql_connect_real_connect(GSQLConnect* obj,GError **);
gboolean g_sql_connect_run_query(GSQLConnect * obj,const gchar * sqlstatement,gsize size);
GSQLResult* g_sql_connect_use_result(GSQLConnect * obj);
gboolean g_sql_connect_ping(GSQLConnect * obj,GError ** err);

extern void (*g_sql_connect_thread_init)();
extern void (*g_sql_connect_thread_end)();



#endif /* GSQLCONNECT_H_ */
