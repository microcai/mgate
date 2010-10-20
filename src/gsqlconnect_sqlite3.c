/*
 * gsqlconnect_sqlite3.c
 *
 *  Created on: 2010-10-18
 *      Author: cai
 */

/**
 * SECTION:gsqlconnect_sqlite3
 * @short_description: GSQLConnectSqlite3 实现 sqlite3 数据库连接
 * @title:数据库连接后端
 * @see_also: #GSQLConnect
 * @stability: Stable
 * @include: monitor/gsqlconnect_sqlite3.h
 *
 * #GSQLConnectSqlite3 是 #GSQLConnect 的一个实现，用来连接到 sqlite 数据库
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <glib.h>
#include <glib-object.h>

#include "i18n.h"
#include "global.h"
#include "gsqlconnect.h"
#include "gsqlconnect_sqlite3.h"
#include "ksql_static_template.h"

enum G_SQL_CONNECT_MYSQL_PROPERTY{
	GSQL_CONNECT_SQLITE_FILE = 1,
};

static void g_sql_connect_sqlite3_class_init(GSQLConnectSqlite3Class * klass);
static void g_sql_connect_sqlite3_init(GSQLConnectSqlite3 * obj);
static void g_sql_connect_sqlite3_finalize(GObject * obj);
static void g_sql_connect_sqlite3_set_property(GObject *object,guint property_id, const GValue *value, GParamSpec *pspec);
static void g_sql_connect_sqlite3_get_property(GObject *object,guint property_id, GValue *value, GParamSpec *pspec);

static gboolean g_sql_connect_sqlite3_real_connect(GSQLConnect * obj,GError ** err);
static gboolean g_sql_connect_sqlite3_ping(GSQLConnect * obj,GError ** );
static gboolean g_sql_connect_sqlite3_check_config(GSQLConnect * obj);
static void g_sql_connect_sqlite3_register_property(GSQLConnectSqlite3Class * klass);
static gboolean	g_sql_connect_sqlite3_real_query(GSQLConnect*,const char * sql_stmt,gsize len /* -1 for nul-terminated string*/);
static gboolean	g_sql_connect_sqlite3_get_row(GSQLResult * obj);
static void g_sql_connect_sqlite3_free_result(GSQLResult * );

G_DEFINE_TYPE(GSQLConnectSqlite3,g_sql_connect_sqlite3,G_TYPE_SQL_CONNNECT);

void g_sql_connect_sqlite3_class_init(GSQLConnectSqlite3Class * klass)
{
	GObjectClass	* gobjclass = G_OBJECT_CLASS(klass);
	GSQLConnectClass * parent_class = (GSQLConnectClass*)klass;
	parent_class->connect = g_sql_connect_sqlite3_real_connect;
	parent_class->check_config = g_sql_connect_sqlite3_check_config;
	gobjclass->set_property = g_sql_connect_sqlite3_set_property;
	gobjclass->get_property = g_sql_connect_sqlite3_get_property;
	parent_class->run_query = g_sql_connect_sqlite3_real_query;
	gobjclass->finalize = g_sql_connect_sqlite3_finalize;
	parent_class->ping = g_sql_connect_sqlite3_ping;

	g_sql_connect_sqlite3_register_property(klass);
}

void g_sql_connect_sqlite3_init(GSQLConnectSqlite3 * obj)
{
}

void g_sql_connect_sqlite3_finalize(GObject * obj)
{
	GSQLConnectSqlite3 * mobj = (GSQLConnectSqlite3*)obj;

	sqlite3_close(mobj->sqlite);
	g_free(mobj->file);
}


gboolean g_sql_connect_sqlite3_real_connect(GSQLConnect * obj,GError ** err)
{
	g_assert(IS_G_SQL_CONNECT_SQLITE(obj));

	GSQLConnectSqlite3 * mobj = (GSQLConnectSqlite3*)obj;

	gchar * file;
	g_object_get(obj,"file",&file,NULL);
	int errcode = sqlite3_open(file,&mobj->sqlite);

	if (errcode!=SQLITE_OK)
	{
		g_error("%s",sqlite3_errmsg(mobj->sqlite));
		sqlite3_close(mobj->sqlite);
		return FALSE;
	}
	return TRUE;
}

gboolean g_sql_connect_sqlite3_ping(GSQLConnect * obj,GError ** err)
{
	g_assert(IS_G_SQL_CONNECT_SQLITE(obj));
	return TRUE;
}

gboolean	g_sql_connect_sqlite3_real_query(GSQLConnect*obj,const char * sql_stmt,gsize len)
{
	int c;
	sqlite3_stmt * stmt;

	g_assert(IS_G_SQL_CONNECT_SQLITE(obj));

	GSQLConnectSqlite3 * mobj = (GSQLConnectSqlite3*)obj;

	sqlite3_prepare(mobj->sqlite,sql_stmt,len,&stmt,NULL);

	if(!stmt)
	{
		obj->lastresult = NULL;
		g_signal_emit_by_name(obj, "query-err",sqlite3_errcode(mobj->sqlite), sqlite3_errmsg(mobj->sqlite));
		return FALSE;
	}

	c = sqlite3_step(stmt);

	if(c == SQLITE_DONE)
	{
		obj->lastresult = NULL;
		return TRUE;
	}

	if (c!=SQLITE_OK)
	{
		sqlite3_finalize(stmt);
		return FALSE;
	}

/*------------------------------------------
	建立 GSQLResult 对象，并存储返回的数据,设置 GSQLresult 虚表使用 sqlite3 特定方法 :)
	*/
	GSQLResult * result =  g_object_new(G_TYPE_SQL_RESULT,
			"type",G_TYPE_SQL_CONNNECT_SQLITE,"result",stmt,
			"fields",sqlite3_column_count(stmt),NULL);

	obj->lastresult = (GObject*)result;

	result->nextrow = g_sql_connect_sqlite3_get_row;

	for(c=0;c <sqlite3_column_count(stmt) ; c++ )
	{
		g_sql_result_append_result_array(result,sqlite3_column_name(stmt,c));
	}

	result->freerows = g_sql_connect_sqlite3_free_result ;

	return TRUE;
}

gboolean	g_sql_connect_sqlite3_get_row(GSQLResult * obj)
{
	int c;
	sqlite3_stmt	* stmt;
	stmt = (sqlite3_stmt*)obj->result;

	c = sqlite3_step(stmt);

//	GStrv row = sqlite3_fetch_row(myres);

//	obj->currow = row;

	return c == SQLITE_OK;
}

gboolean	g_sql_connect_sqlite3_seek_row(GSQLResult * obj,guint offset)
{
	int c;
	sqlite3_stmt	* stmt;
	stmt = (sqlite3_stmt*)obj->result;

	while(offset--)
		c  = sqlite3_step(stmt);

	return (c==SQLITE_OK);
}

void g_sql_connect_sqlite3_free_result(GSQLResult * obj)
{
	sqlite3_finalize(obj->result);
}

#define INSTALL_PROPERTY_STRING(klass,id,name,defaultval,type) \
		g_object_class_install_property(klass,id, \
			g_param_spec_string(name,name,name,defaultval,type))

void g_sql_connect_sqlite3_register_property(GSQLConnectSqlite3Class * klass)
{
	GObjectClass* objclass =(GObjectClass*) klass ;
	INSTALL_PROPERTY_STRING(objclass,GSQL_CONNECT_SQLITE_FILE,"file",NULL,G_PARAM_CONSTRUCT|G_PARAM_READWRITE);
}

void g_sql_connect_sqlite3_set_property(GObject *object, guint property_id,
		const GValue *value, GParamSpec *pspec)
{
	GSQLConnectSqlite3 * mobj = (GSQLConnectSqlite3*) object;

	g_return_if_fail(IS_G_SQL_CONNECT_SQLITE(mobj));

	switch (property_id)
	{
	case GSQL_CONNECT_SQLITE_FILE:
		g_free(mobj->file);
		mobj->file = g_value_dup_string(value);
		break;
	default:
		g_warn_if_reached();
		break;
	}
}

void g_sql_connect_sqlite3_get_property(GObject *object,
		guint property_id, GValue *value, GParamSpec *pspec)
{
	GSQLConnectSqlite3 * mobj = (GSQLConnectSqlite3*)object;

	g_return_if_fail(IS_G_SQL_CONNECT_SQLITE(mobj));

	switch (property_id)
	{
	case GSQL_CONNECT_SQLITE_FILE:
		g_value_set_static_string(value, mobj->file);
		break;
	default:
		g_warn_if_reached();
		break;
	}
}

gboolean g_sql_connect_sqlite3_check_config(GSQLConnect * obj)
{
	g_assert(IS_G_SQL_CONNECT_SQLITE(obj));

	gchar * g_file = g_key_file_get_string(gkeyfile,"sqlite","file",NULL);

	if (g_file)
	{
		g_strchomp(g_strchug(g_file));
		if (strlen(g_file))
		{
			g_object_set(obj,"file",g_file,NULL);
		}
	}
	g_free(g_file);
	return TRUE;
}
