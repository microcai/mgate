/*
 * gsqlresult.c
 *
 *  Created on: 2010-4-14
 *      Author: cai
 */


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

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
};

void g_sql_result_set_property(GObject *object, guint property_id,const GValue *value, GParamSpec *pspec);
void g_sql_result_get_property(GObject *object, guint property_id,GValue *value, GParamSpec *pspec);

static void g_sql_result_dispose(GObject * obj)
{

}


static void g_sql_result_finalize(GObject * obj)
{

}

static void g_sql_result_class_init(GSQLResultClass * klass)
{
	GObjectClass	* gobjclass = G_OBJECT_CLASS(klass);
	gobjclass->finalize = g_sql_result_finalize;
	gobjclass->dispose = g_sql_result_dispose;
	gobjclass->set_property = g_sql_result_set_property;
	gobjclass->get_property = g_sql_result_get_property;

	g_object_class_install_property(gobjclass,G_SQL_RESULT_PROPERTY_RESULT,g_param_spec_pointer("result","result","result",G_PARAM_CONSTRUCT_ONLY|G_PARAM_READABLE));
	g_object_class_install_property(gobjclass,G_SQL_RESULT_PROPERTY_TYPE,g_param_spec_gtype("type","type","type",G_TYPE_SQL_CONNNECT,G_PARAM_CONSTRUCT_ONLY|G_PARAM_READABLE));
}

static void g_sql_result_init(GSQLResult * obj)
{

}

G_DEFINE_TYPE(GSQLResult,g_sql_result,G_TYPE_SQL_RESULT);

void	g_sql_result_set_result_array(GSQLResult * obj, const gchar * first , gsize offset, ... )
{

}

GStrv	g_sql_result_next_row()
{


}

GStrv	g_sql_result_seek_row(guint		rowtoseek)
{

}

const gchar* g_sql_result_colum_by_name(const gchar * columname)
{

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
	default:
		g_warn_if_reached();
		break;
	}
}
