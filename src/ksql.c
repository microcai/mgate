/*
 * ksql.c
 *
 *  Created on: 2010-4-10
 *      Author: cai
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



#include "i18n.h"
#include "global.h"
#include "ksql.h"
#include "gsqlconnect.h"
#include "gsqlconnect_mysql.h"

static GAsyncQueue * asqueue;
static GSQLConnect * connector;

static gpointer ksql_thread(gpointer user_data)
{
//	g_sql_connect_thread_init();
	//	mysql_commit()
	GError * err = NULL;
	GSQLConnect * connector = (typeof(connector)) user_data;

	while(!g_sql_connect_real_connect(connector,&err))
	{
		g_warning(_("unable to connect to database server (%d): %s"),err->code,err->message);
		g_error_free(err);
		err = NULL;
		sleep(2);
	}

	GSQLResult * res;

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
	g_sql_connect_ping(G_SQL_CONNECT(user_data),NULL);
	return TRUE;
}

GType	ksql_get_backend()
{
	g_assert(gkeyfile);

	//make sure it present

	GType	backend = 0;

	gchar * bk = g_key_file_get_string(gkeyfile,"database","backend",NULL);

	if (bk)
	{
#if HAVE_MYSQL
		if (g_strcmp0(g_strchomp(g_strchug(bk)), "mysql") == 0)
		{
			g_free(bk);
			backend = G_TYPE_SQL_CONNNECT_MYSQL;
		}
		else
#endif
		if(g_strcmp0(g_strchomp(g_strchug(bk)), "sqlite") == 0)
		{
			g_free(bk);
			//backend = "GSQLConnectSqlite";
		}else
		{
			g_free(bk);
			bk = NULL;
		}
	}
	if(!bk)
	{
#ifdef HAVE_MYSQL
		g_message(_("[database]:[backend] not set or invalid, default to mysql"));
		backend = G_TYPE_SQL_CONNNECT_MYSQL;
#else
		g_message(_("[database]:[backend] not set or invalid, default to sqlite"));
		backend = "GSQLConnectSqlite";
#endif
	}
	return backend ;
}



void	ksql_init(gboolean createdb)
{
	asqueue = g_async_queue_new_full(g_free);

	GType	backend =  ksql_get_backend();


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
