/*
 * gsqlconnect.c
 *
 *  Created on: 2010-4-13
 *      Author: cai
 */


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <glib.h>
#include <glib-object.h>

#include "i18n.h"
#include "gsqlconnect.h"

static void dumy_func(){}

void (*g_sql_connect_thread_init)() = { dumy_func };
void (*g_sql_connect_thread_end)() = { dumy_func };
static void g_marshal_VOID__INT_STRING(GClosure *closure, GValue *return_value,
		guint n_param_values, const GValue *param_values,
		gpointer invocation_hint, gpointer marshal_data);
static void g_sql_connect_result_weak_notify(gpointer data,GObject *where_the_object_was);

static void g_sql_connect_class_init(GSQLConnectClass * klass)
{
	g_signal_new("query_err",G_TYPE_FROM_CLASS(klass),G_SIGNAL_RUN_LAST,0,NULL,NULL,g_cclosure_marshal_VOID__STRING,G_TYPE_NONE,2,G_TYPE_INT,G_TYPE_STRING);
}

static void g_sql_connect_init(GSQLConnect * klass)
{

}


G_DEFINE_TYPE(GSQLConnect,g_sql_connect,G_TYPE_OBJECT);

gboolean g_sql_connect_check_config(GSQLConnect* obj)
{
	g_return_val_if_fail(IS_G_SQL_CONNECT(obj),FALSE);

	GSQLConnectClass * klass = G_SQL_CONNECT_GET_CLASS(obj) ;

	if(klass->check_config)
	{
		return klass->check_config(obj);
	}
	return FALSE;
}

gboolean g_sql_connect_real_connect(GSQLConnect* obj,GError ** err)
{
	g_return_val_if_fail(IS_G_SQL_CONNECT(obj),FALSE);

	GSQLConnectClass * klass = G_SQL_CONNECT_GET_CLASS(obj) ;

	if(klass->connect)
	{
		return klass->connect(obj,err);
	}
	return FALSE;
}

gboolean g_sql_connect_run_query(GSQLConnect * obj,const gchar * sqlstatement,gsize size)
{
	g_return_val_if_fail(IS_G_SQL_CONNECT(obj),FALSE);

	GSQLConnectClass * klass = G_SQL_CONNECT_GET_CLASS(obj) ;

	register	gboolean ret = FALSE;

	if (obj->lastresult)
	{
		g_object_unref(obj->lastresult);
		obj->lastresult = NULL;
	}

	if(klass->run_query)
	{
		ret = klass->run_query(obj,sqlstatement,size);
	}

	if(ret)
	{
		g_object_weak_ref((GObject*)obj->lastresult,g_sql_connect_result_weak_notify,obj);
	}

	return ret;
}

GSQLResult* g_sql_connect_use_result(GSQLConnect * obj)
{
	g_return_val_if_fail(IS_G_SQL_CONNECT(obj),NULL);
	return G_SQL_RESULT(obj->lastresult);
}

gboolean g_sql_connect_ping(GSQLConnect * obj,GError ** err)
{
	g_return_val_if_fail(IS_G_SQL_CONNECT(obj),FALSE);

	GSQLConnectClass * klass = G_SQL_CONNECT_GET_CLASS(obj) ;

	if(klass->ping)
	{
		return klass->ping(obj,err);
	}
	return FALSE;
}

void g_sql_connect_result_weak_notify(gpointer data,GObject *where_the_object_was)
{
	GSQLConnect * sql_connect = (GSQLConnect*) data;
	if (sql_connect->lastresult == where_the_object_was)
	{
		sql_connect->lastresult = NULL;
	}
}


void g_marshal_VOID__INT_STRING(GClosure *closure, GValue *return_value G_GNUC_UNUSED,
		guint n_param_values, const GValue *param_values,
		gpointer invocation_hint G_GNUC_UNUSED, gpointer marshal_data)
{
	typedef void (*GMarshalFunc_VOID__INT_STRING)(gpointer data1, gint arg_1,
			const gchar * arg_2, gpointer data2);
	register GMarshalFunc_VOID__INT_STRING callback;
	register GCClosure *cc = (GCClosure*) closure;
	register gpointer data1, data2;

	g_return_if_fail (n_param_values == 3);

	if (G_CCLOSURE_SWAP_DATA (closure))
	{
		data1 = closure->data;
		data2 = g_value_peek_pointer(param_values + 0);
	}
	else
	{
		data1 = g_value_peek_pointer(param_values + 0);
		data2 = closure->data;
	}
	callback = (GMarshalFunc_VOID__INT_STRING) (marshal_data ? marshal_data
			: cc->callback);

	callback(data1, g_value_get_int(param_values + 1),
			g_value_get_string(param_values + 2), data2);
}
