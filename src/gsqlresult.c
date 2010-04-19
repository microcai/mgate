/*
 * gsqlresult.c
 *
 *  Created on: 2010-4-14
 *      Author: cai
 */


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdarg.h>
#include <sys/syslog.h>
#include <sys/socket.h>
#include <net/ethernet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <pcap.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <pcap/pcap.h>
#include <syslog.h>
#include <string.h>
#include <glib.h>
#include <glib-object.h>

#include "gsqlresult.h"
#include "gsqlconnect.h"

enum{
	G_SQL_RESULT_PROPERTY_TYPE = 1,
	G_SQL_RESULT_PROPERTY_TYPE_INSTANCE ,
	G_SQL_RESULT_PROPERTY_RESULT ,
	G_SQL_RESULT_PROPERTY_FIELDS ,

};

void g_sql_result_set_property(GObject *object, guint property_id,const GValue *value, GParamSpec *pspec);
void g_sql_result_get_property(GObject *object, guint property_id,GValue *value, GParamSpec *pspec);

static void g_sql_result_dispose(GObject * obj)
{

}

static void g_sql_result_finalize(GObject * gobj)
{
	GSQLResult * obj = (GSQLResult*)gobj;
	g_ptr_array_free(obj->colum,TRUE);
}

static void g_sql_result_class_init(GSQLResultClass * klass)
{
	GObjectClass	* gobjclass = G_OBJECT_CLASS(klass);
	gobjclass->finalize = g_sql_result_finalize;
	gobjclass->dispose = g_sql_result_dispose;
	gobjclass->set_property = g_sql_result_set_property;
	gobjclass->get_property = g_sql_result_get_property;

	g_signal_new("destory",G_OBJECT_CLASS_TYPE(klass),G_SIGNAL_RUN_CLEANUP,0,0,0,g_cclosure_marshal_VOID__INT,G_TYPE_NONE,0);

	g_object_class_install_property(gobjclass,G_SQL_RESULT_PROPERTY_RESULT,g_param_spec_pointer("result","result","result",G_PARAM_CONSTRUCT_ONLY|G_PARAM_READABLE));
	g_object_class_install_property(gobjclass,G_SQL_RESULT_PROPERTY_TYPE,g_param_spec_gtype("type","type","type",G_TYPE_SQL_CONNNECT,G_PARAM_CONSTRUCT_ONLY|G_PARAM_READABLE));
	g_object_class_install_property(gobjclass,G_SQL_RESULT_PROPERTY_FIELDS,g_param_spec_int("fields","fields","fields",0,99,0,G_PARAM_CONSTRUCT_ONLY|G_PARAM_READABLE));

}

static void g_sql_result_init(GSQLResult * obj)
{
	obj->colum = g_ptr_array_new_with_free_func(g_free);
}

G_DEFINE_TYPE(GSQLResult,g_sql_result,G_TYPE_SQL_RESULT);

void	g_sql_result_set_result_array(GSQLResult * obj, ...)
{

	const gchar * charptr;
	va_list	v;
	va_start(v,obj);

	while( charptr = va_arg(v,gchar*) )
	{
		gchar * pstr = strdup(charptr);
		g_ptr_array_add(obj->colum,pstr);
	}
	va_end(v);
}

void	g_sql_result_append_result_array(GSQLResult * obj, const char * field)
{
	gchar * pstr = strdup(field);
	g_ptr_array_add(obj->colum,pstr);
}

GStrv	g_sql_result_get_row(GSQLResult * obj)
{
	return obj->currow;
}

gboolean g_sql_result_next_row(GSQLResult * obj)
{
	return obj->nextrow(obj);
}

gboolean g_sql_result_seek_row(GSQLResult * obj,guint	rowtoseek)
{
	return obj->seekrow(obj,rowtoseek);
}

const gchar* g_sql_result_colum(GSQLResult * obj,const guint index)
{
	return obj->currow[index];
}

const gchar* g_sql_result_colum_by_name(GSQLResult * obj,const gchar * columname)
{
	int i;
	gchar * str;
	for(i=0;i<obj->colum->len;i++)
	{
		str = (char*)g_ptr_array_index(obj->colum,i);
		if(g_strcmp0(str,columname)==0)
		{
			return g_sql_result_colum(obj,i);
		}
	}
}

void g_sql_result_set_property(GObject *object, guint property_id,const GValue *value, GParamSpec *pspec)
{
	GSQLResult * mobj = (GSQLResult*) object;

	g_return_if_fail(IS_G_SQL_RESULT(mobj));

	switch (property_id)
	{
	case G_SQL_RESULT_PROPERTY_TYPE:
		mobj->connector_type = g_value_get_gtype(value);
		break;
	case G_SQL_RESULT_PROPERTY_TYPE_INSTANCE:
		mobj->connector = g_value_get_object(value);
		break;
	case G_SQL_RESULT_PROPERTY_RESULT:
		mobj->result = g_value_get_pointer(value);
		break;
	case G_SQL_RESULT_PROPERTY_FIELDS:
		mobj->fields = g_value_get_int(value);
	default:
		g_warn_if_reached();
		break;
	}
}

void g_sql_result_get_property(GObject *object, guint property_id,GValue *value, GParamSpec *pspec)
{
	GSQLResult * mobj = (GSQLResult*) object;
	g_return_if_fail(IS_G_SQL_RESULT(mobj));

	switch (property_id)
	{
	case G_SQL_RESULT_PROPERTY_TYPE:
		g_value_set_gtype(value, mobj->connector_type);
		break;
	case G_SQL_RESULT_PROPERTY_TYPE_INSTANCE:
		g_value_set_object(value, mobj->connector);
		break;
	case G_SQL_RESULT_PROPERTY_RESULT:
		g_value_set_pointer(value, mobj->result);
		break;
	case G_SQL_RESULT_PROPERTY_FIELDS:
		g_value_set_int(value,mobj->fields);
		break;
	default:
		g_warn_if_reached();
		break;
	}
}
