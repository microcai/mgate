/*
 * gsqlresult.c
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

/**
 * SECTION:gsqlresult
 * @short_description: #GSQLResult 表征了一个查询结果集
 * @title:数据库查询结果
 * @see_also: #GSQLConnect
 * @stability: Stable
 * @include: monitor/gsqlconnect.h
 *
 * #GSQLConnect 是 一个通用的 SQL 数据库 glib 绑定库，通过插件可以支持很多后端
 * 目前只实现了mysql 后端  #GSQLConnectMysql
 *
 * #GSQLResult 表征了一个查询结果集，所有 #GSQLConnect 后端查询结果都使用 #GSQLResult
 * 返回， #GSQLResult 提供了对结果的优雅的使用
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdarg.h>
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
static void g_sql_result_class_init(GSQLResultClass * klass);
static void g_sql_result_init(GSQLResult * obj);


G_DEFINE_TYPE(GSQLResult,g_sql_result,G_TYPE_OBJECT);

static void g_sql_result_finalize(GObject * gobj)
{
	GSQLResult * obj = (GSQLResult*)gobj;
	GObjectClass* klass = G_OBJECT_CLASS(g_sql_result_parent_class);

	g_ptr_array_free(obj->colum,TRUE);

	obj->freerows(obj);
	g_signal_emit_by_name(gobj,"destory");
	if(klass->finalize)
		klass->finalize(gobj);
}

static void g_sql_result_dispose(GObject * gobj)
{
	GSQLResult * obj = (GSQLResult*)gobj;
	GObjectClass* klass = G_OBJECT_CLASS(g_sql_result_parent_class);

	g_signal_emit_by_name(obj,"destory");

	if(klass->dispose)
		klass->dispose(gobj);
}


void g_sql_result_class_init(GSQLResultClass * klass)
{
	GObjectClass	* gobjclass = G_OBJECT_CLASS(klass);
	gobjclass->finalize = g_sql_result_finalize;
	gobjclass->dispose = g_sql_result_dispose;
	gobjclass->set_property = g_sql_result_set_property;
	gobjclass->get_property = g_sql_result_get_property;

	g_signal_new("destory",G_OBJECT_CLASS_TYPE(klass),G_SIGNAL_RUN_LAST,0,0,0,g_cclosure_marshal_VOID__VOID,G_TYPE_NONE,0);

	g_object_class_install_property(gobjclass,G_SQL_RESULT_PROPERTY_RESULT,g_param_spec_pointer("result","result","result",G_PARAM_CONSTRUCT_ONLY|G_PARAM_READWRITE));
	g_object_class_install_property(gobjclass,G_SQL_RESULT_PROPERTY_TYPE,g_param_spec_gtype("type","type","type",G_TYPE_SQL_CONNNECT,G_PARAM_CONSTRUCT_ONLY|G_PARAM_READWRITE));
	g_object_class_install_property(gobjclass,G_SQL_RESULT_PROPERTY_FIELDS,g_param_spec_int("fields","fields","fields",0,99,0,G_PARAM_CONSTRUCT_ONLY|G_PARAM_READWRITE));

}

void g_sql_result_init(GSQLResult * obj)
{
	obj->colum = g_ptr_array_new_with_free_func(g_free);
}

/**
 * g_sql_result_set_result_array:
 * @obj: An  #GSQLResult
 * @... : 字段列表
 *
 * g_sql_result_set_result_array() 用来设置字段先后循序，这样后续使用的时候就能直接用字段名
 * 来引用字段。这是提供给 #GSQLConnect 的实现使用的
 */
void	g_sql_result_set_result_array(GSQLResult * obj, ...)
{

	const gchar * charptr;
	va_list	v;
	va_start(v,obj);

	while(( charptr = va_arg(v,gchar*) ))
	{
		gchar * pstr = strdup(charptr);
		g_ptr_array_add(obj->colum,pstr);
	}
	va_end(v);
}
/**
 * g_sql_result_append_result_array:
 * @obj: An  #GSQLResult
 * @field: 字段名
 *
 * 将字段名加入列表，这样后续使用的时候就能直接用字段名来引用字段。这是提供给 #GSQLConnect 的实现使用的
 */
void	g_sql_result_append_result_array(GSQLResult * obj, const char * field)
{
	gchar * pstr = strdup(field);
	g_ptr_array_add(obj->colum,pstr);
}

/**
 * g_sql_result_get_row:
 * @obj : An  #GSQLResult
 *
 * 得到当前行。使用 g_sql_result_fetch_row() 来获取下一行
 */
GStrv	g_sql_result_get_row(GSQLResult * obj)
{
	return obj->currow;
}

/**
 * g_sql_result_fetch_row:
 * @obj : An  #GSQLResult
 *
 * 抽取下一行。g_sql_result_fetch_row() 必须在使用任何结果前调用
 * 如果米有更多结果了，g_sql_result_fetch_row 返回 FALSE
 *
 * Returns: FALSE 如果没有更多结果了
 */
gboolean g_sql_result_fetch_row(GSQLResult * obj)
{
	g_return_val_if_fail(IS_G_SQL_RESULT(obj),FALSE);
	return obj->nextrow(obj);
}

/**
 * g_sql_result_seek_row:
 * @obj:An  #GSQLResult
 * @rowtoseek: 跳过的行数
 *
 * 跳过一定的行不处理
 *
 * Returns: FALSE 如果失败
 */
gboolean g_sql_result_seek_row(GSQLResult * obj,guint	rowtoseek)
{
	g_return_val_if_fail(IS_G_SQL_RESULT(obj),FALSE);
	return obj->seekrow(obj,rowtoseek);
}

/**
 * g_sql_result_colum:
 * @obj:
 * @index: 第几列
 *
 *
 * Returns: 当前行指定列的字符串，不必释放
 */
const gchar* g_sql_result_colum(GSQLResult * obj,const guint index)
{
	return obj->currow[index];
}

/**
 * g_sql_result_colum_by_name:
 * @obj:
 * @columname: 列名字
 *
 * Returns: 当前行指定列的字符串，不必释放
 */
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
	return NULL;
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
	case G_SQL_RESULT_PROPERTY_FIELDS:
		g_value_set_int(value,mobj->fields);
		break;
	default:
		g_warn_if_reached();
		break;
	}
}
