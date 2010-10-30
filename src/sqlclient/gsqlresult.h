/*
 * gsqlresult.h
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

#ifndef GSQLRESULT_H_
#define GSQLRESULT_H_

#include <glib.h>
#include <glib-object.h>

typedef struct _GSQLResult GSQLResult;


struct _GSQLResult{
	GObject parent;
	GType connector_type;
	GObject *connector;
	gpointer result;
	GPtrArray *colum;
	GStrv	   currow;
	gint	   fields;
	gboolean (*nextrow)(GSQLResult *);
	gboolean (*seekrow)(GSQLResult *,guint);
	void	 (*freerows)(GSQLResult*);
};

typedef struct _GSQLResultClass{
	GObjectClass parent_class;
}GSQLResultClass;

#define G_TYPE_SQL_RESULT	g_sql_result_get_type()
#define G_SQL_RESULT(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), G_TYPE_SQL_RESULT, GSQLResult))
#define G_SQL_RESULT_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), G_TYPE_SQL_RESULT, GSQLResultClass))
#define IS_G_SQL_RESULT(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), G_TYPE_SQL_RESULT))
#define IS_G_SQL_RESULT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), G_TYPE_SQL_RESULT))
#define G_SQL_RESULT_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), G_TYPE_SQL_RESULT, GSQLResultClass))


GType	g_sql_result_get_type() G_GNUC_CONST;

/* Don't free the returned results!*/
GStrv	g_sql_result_get_row(GSQLResult * obj);

gboolean	g_sql_result_fetch_row(GSQLResult * obj);

gboolean g_sql_result_seek_row(GSQLResult * obj,guint	rowtoseek);

void	g_sql_result_set_result_array(GSQLResult * obj , ...  /* const gchar * first , ...*/ );
void	g_sql_result_append_result_array(GSQLResult * obj, const char * field);
const gchar* g_sql_result_colum_by_name(GSQLResult * obj,const gchar * columname);
const gchar* g_sql_result_colum(GSQLResult * obj,const guint index);

#endif /* GSQLRESULT_H_ */
