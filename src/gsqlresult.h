/*
 * gsqlresult.h
 *
 *  Created on: 2010-4-14
 *      Author: cai
 */

#ifndef GSQLRESULT_H_
#define GSQLRESULT_H_

#include <glib.h>
#include <glib-object.h>

#ifndef GSQLConnect

typedef struct _GSQLConnect GSQLConnect;

#define GSQLConnect GSQLConnect

#endif

typedef struct _GSQLResult{
	GObject	parent;
	GType	connector_type;
	GSQLConnect	*connector;
	gpointer	 result;
}GSQLResult;

typedef struct _GSQLResultClass{
	GObjectClass parent_class;
}GSQLResultClass;

#define G_TYPE_SQL_RESULT	g_sql_result_get_type()
#define G_SQL_RESULT(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), G_TYPE_SQL_CONNNECT, GSQLResult))
#define G_SQL_RESULT_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), G_TYPE_SQL_RESULT, GSQLResultClass))
#define IS_G_SQL_RESULT(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), G_TYPE_SQL_RESULT))
#define IS_G_SQL_RESULT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), G_TYPE_SQL_RESULT))
#define G_SQL_RESULT_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), G_TYPE_SQL_RESULT, GSQLResultClass))


GType	g_sql_result_get_type() G_GNUC_CONST;

/* Don't free the returned results!*/
GStrv	g_sql_result_next_row();

void	g_sql_result_set_result_array(GSQLResult * , const gchar * first , gsize offset, ... );

#endif /* GSQLRESULT_H_ */
