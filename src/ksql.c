/*
 * ksql.c
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


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <sys/epoll.h>
#include <sys/syslog.h>

#include <pthread.h>
#include <signal.h>
#include <errno.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <memory.h>
#include <stdlib.h>
#include <stdio.h>
#include <glib.h>


#include "utils.h"
#include "i18n.h"
#include "global.h"
#include "ksql.h"
#include "gsqlconnect.h"
#include "gsqlresult.h"
#include "clientmgr.h"


static GAsyncQueue * asqueue;
static GSQLConnect * connector;

static gpointer ksql_thread(gpointer user_data)
{
//	g_sql_connect_thread_init();
	//	mysql_commit()
	GSQLResult * res;
	GError * err = NULL;
	GSQLConnect * connector = (typeof(connector)) user_data;

	//TODO : 首先是在 monitor.cfg 指示的 白名单 :)

	while(!g_sql_connect_real_connect(connector,&err))
	{
		if(err)
			g_warning(_("unable to connect to database server (%d): %s"),err->code,err->message);
		else
		{
			g_warning(_("sqlite backend not implemented, exiting!"));
			for(;;)
			{
				g_free(g_async_queue_pop(asqueue));
			}
		}
		g_error_free(err);
		err = NULL;
		sleep(2);
	}

	//load white list sqlite 数据库没有白名单制度，(*^__^*) 嘻嘻……
	res = ksql_query("select MAC_ADDR from whitelist LIMIT 0,1000");
	while(g_sql_result_fetch_row(res))
	{
		const gchar * mac = g_sql_result_colum_by_name(res,"MAC_ADDR");

		u_char mac_addr[6];

		convertMAC(mac_addr,mac);

		//将 mac 加入许可范围 :)

		Client * dummyclient = client_new("","","",mac_addr);

		dummyclient->enable = TRUE;

		clientmgr_insert_client_by_mac(mac_addr,dummyclient);
	}
	if(res)
		g_object_unref(res);
	//还有呢！预先加载一些,目前是不需要了 :D
	//预加载已经存在的客户端



	gchar * sql;
	for (;;)
	{
		sql = (gchar*) g_async_queue_pop(asqueue);
		res = ksql_query_free_str(sql);

		if(res)
			g_object_unref(res);
	}
	g_sql_connect_thread_end();
	return NULL;
}

static gboolean ksql_ping(gpointer user_data)
{
	return g_sql_connect_ping(G_SQL_CONNECT(user_data),NULL);
}

void	ksql_init(gboolean createdb)
{
	asqueue = g_async_queue_new_full(g_free);

	GType	backend =  sqlconnect_get_backend();


	connector = (GSQLConnect*)g_object_new(backend,NULL);

	g_sql_connect_check_config(connector);

	if(createdb)
	{
		GError * err = NULL;

		if(!g_sql_connect_real_connect(connector,&err))
		{
			g_error(_("unable to connect to database server (%d): %s"),err->code,err->message);
		}
	}else
	{
		g_thread_create(ksql_thread,connector,0,0);
		g_timeout_add_seconds(30,ksql_ping,connector);
	}
}

GSQLResult * ksql_query(const gchar * stm)
{
	if (g_sql_connect_run_query(connector, stm, strlen(stm)))
	{
		return g_sql_connect_use_result(connector);
	}
	return NULL;
}

GSQLResult * ksql_query_free_str(gchar * stm)
{
	GSQLResult * ret = ksql_query(stm);
	g_free(stm);
	return ret;
}

GSQLResult * ksql_vquery(const gchar * stmformat , ...)
{
	va_list v;
	gchar * stm ;
	va_start(v,stmformat);
	stm = g_strdup_vprintf(stmformat,v);
	va_end(v);
	return ksql_query_free_str(stm);
}

void ksql_query_async_free_str(const gchar * stm)
{
	g_async_queue_push(asqueue,(gpointer)stm);
}

void ksql_query_async(const gchar * stm)
{
	g_async_queue_push(asqueue,g_strdup(stm));
}

void ksql_vquery_async(const gchar * stmformat , ...)
{
	va_list v;
	gchar * stm ;
	va_start(v,stmformat);
	stm = g_strdup_vprintf(stmformat,v);
	va_end(v);
	ksql_query_async_free_str(stm);
}
