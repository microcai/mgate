/*
 * gsqlconnect.c
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
 * SECTION:gsqlconnect
 * @short_description: GSQLConnect 实现通用的数据库连接
 * @title:数据库连接后端
 * @see_also: #GSQLConnect
 * @stability: Stable
 * @include: monitor/gsqlconnect.h
 *
 * #GSQLConnect 是 一个通用的 SQL 数据库 glib 绑定库，通过插件可以支持很多后端
 * 目前只实现了mysql 后端  #GSQLConnectMysql
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <glib.h>
#include <glib-object.h>

#include <glib/gi18n.h>
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
	/**
	 * GSQLConnect::query-err:
	 * @gsqlconnect: 接收信号的 #GSQLConnect 物体
	 * @errcode:	错误代号
	 * @errstr:	具体的错误描述
	 *
	 * 如果查询结果出错，::query-err 信号将会发出
	 */
	g_signal_new("query-err",G_TYPE_FROM_CLASS(klass),G_SIGNAL_RUN_LAST,0,NULL,NULL,
			g_marshal_VOID__INT_STRING,
			G_TYPE_NONE,2,G_TYPE_INT,G_TYPE_STRING,NULL);
}

static void g_sql_connect_init(GSQLConnect * klass)
{

}


G_DEFINE_TYPE(GSQLConnect,g_sql_connect,G_TYPE_OBJECT);

/**
 * g_sql_connect_check_config:
 * @obj : 连接
 * Returns : TRUE 成功，FALSE 失败
 * g_sql_connect_real_connect() 被后端重载，给后端一个机会在连接前检查配置
 * 用户应该在调用 g_sql_connect_real_connect() 前调用 g_sql_connect_real_connect()
 * 以确认数据库连接配置的正确性
 */
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

/**
 * g_sql_connect_real_connect:
 * @obj :
 * @err :
 * Returns : TRUE 成功，FALSE 失败, 具体错误可以查看 @err
 *
 * 连接到数据库。数据库在正确使用前必须连接。
 */
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

/**
 * g_sql_connect_run_query:
 * @obj :
 * @sqlstatement : SQL 语句 NULL 结尾
 * @size : SQL 语句长度
 * Returns : TRUE 成功，FALSE 失败.
 *
 * 执行一个 SQL 语句。
 */
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

	if(ret && obj->lastresult)
	{
		g_object_weak_ref((GObject*)obj->lastresult,g_sql_connect_result_weak_notify,obj);
	}

	return ret;
}

/**
 * g_sql_connect_use_result:
 * @obj:
 * Returns : #GSQLResult
 *
 * 取得最近的一次查询的结果。
 */
GSQLResult* g_sql_connect_use_result(GSQLConnect * obj)
{
	g_return_val_if_fail(IS_G_SQL_CONNECT(obj),NULL);
	return G_SQL_RESULT(obj->lastresult);
}

/**
 * g_sql_connect_ping:
 * @obj : #GSQLConnect
 * @err : 使用 #GError * err = NULL ; &err 传入
 * Returns : TRUE 成功，FALSE 失败.
 *
 * 测试连接是否失效，如果失效，立即重连
 */
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

