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

#if defined HAVE_MYSQL_MYSQL_H || defined HAVE_MYSQL_H
#include <mysql/mysql.h>
#endif

#include "i18n.h"
#include "global.h"
#include "ksql_static_template.h"
#include "ksql.h"

static MYSQL	mysql[1];
static const gchar *	user = "root";
static const gchar * 	passwd = "";
static const gchar * 	db = "hotel";
GAsyncQueue	*			asqueue;


static gpointer ksql_thread(gpointer user_data)
{
//	mysql_commit()

	mysql_real_connect(mysql,NULL,user,passwd,db,0,0,0);

	if(mysql_errno(mysql) == 4049)
	{

	}

	gchar * sql;
	for (;;)
	{
		sql = (gchar*) g_async_queue_pop(asqueue);
		mysql_query(mysql,sql);
		g_free(sql);
	}
	return NULL;
}

void	ksql_init()
{
	mysql_thread_init();
	mysql_init(mysql);
	g_assert(gkeyfile);

	gchar * g_db = g_key_file_get_string(gkeyfile,"mysql","db",NULL);
	gchar * g_user = g_key_file_get_string(gkeyfile,"mysql","user",NULL);
	gchar * g_passwd = g_key_file_get_string(gkeyfile,"mysql","passwd",NULL);

	if (g_db)
	{
		g_strchomp(g_strchug(g_user));
		if (strlen(g_db))
			db = g_db;
	}

	if(g_user)
	{
		g_strchomp(g_strchug(g_user));
		user = g_user ;
	}
	if(g_passwd)
	{
		g_strchomp(g_strchug(g_passwd));
		user = g_passwd ;
	}

	asqueue = g_async_queue_new_full(g_free);

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
