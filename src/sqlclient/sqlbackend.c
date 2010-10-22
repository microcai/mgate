/*
 * sqlbackend.c
 *
 *  Created on: 2010-10-22
 *      Author: cai
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


#include "i18n.h"
#include "gsqlconnect.h"
#include "gsqlconnect_mysql.h"
#include "gsqlconnect_sqlite3.h"
#include "global.h"

GType	sqlconnect_get_backend()
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
#endif
#ifdef WITH_SQLITE3
		g_message(_("[database]:[backend] not set or invalid, default to sqlite"));
		backend = G_TYPE_SQL_CONNNECT_SQLITE;
#endif
	}
	return backend ;
}
