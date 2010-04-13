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
#include "ksql_static_template.h"
#include "ksql.h"
#include "gsqlconnect.h"
#include "gsqlconnect_mysql.h"

static MYSQL	mysql[1];
static const gchar *	user = "root";
static const gchar * 	passwd = "";
static const gchar * 	db = "hotel";
GAsyncQueue	*			asqueue;


static gpointer ksql_thread(gpointer user_data)
{
//	mysql_commit()
	GSQLConnect * connector;


	gchar * sql;
	for (;;)
	{
		sql = (gchar*) g_async_queue_pop(asqueue);
	//	mysql_query(mysql,sql);
		g_free(sql);
	}
	return NULL;
}

void	ksql_init()
{
	mysql_thread_init();
	mysql_init(mysql);
	g_assert(gkeyfile);

	//make sure it present
	G_TYPE_SQL_CONNNECT;
#ifdef  HAVE_MYSQL
	G_TYPE_SQL_CONNNECT_MYSQL ;
#endif

	const gchar * backend = NULL;

	gchar * bk = g_key_file_get_string(gkeyfile,"database","backend",NULL);

	if (bk)
	{
#if HAVE_MYSQL
		if (g_strcmp0(g_strchomp(g_strchug(bk)), "mysql") == 0)
		{
			g_free(bk);
			backend = "GSQLConnectMysql";
			 ;
		}
		else
#endif
		if(g_strcmp0(g_strchomp(g_strchug(bk)), "sqlite") == 0)
		{
			g_free(bk);
			backend = "GSQLConnectSqlite";
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
		backend = "GSQLConnectMysql";
#else
		g_message(_("[database]:[backend] not set or invalid, default to sqite"));
		backend = "GSQLConnectSqlite";
#endif
	}

	asqueue = g_async_queue_new_full(g_free);

	GType sqlconnector =  g_type_from_name(backend);
//	g_object_new(G_TYP);


	g_thread_create(ksql_thread,0,0,0);
}

//打开并连接到数据库, sqlite or mysql?
gboolean	ksql_connect_sql()
{



	//mysql_real_connect(&mysql,NULL,);

}

gboolean	ksql_connect_sql_assync()
{
	g_assert(gkeyfile);

//	mysql_real_connect(&mysql,)


	//mysql_real_connect(&mysql,NULL,);

}


void ksql_create_db()
{
	for (int i = 0; i < (int) (sizeof(create_sql) / sizeof(char*)); ++i)
		mysql_query(mysql, create_sql[i]);
}
