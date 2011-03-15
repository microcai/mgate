/*
 * sqlbackend.c
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


#include <glib/gi18n.h>
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
#else
#ifdef WITH_SQLITE3
		g_message(_("[database]:[backend] not set or invalid, default to sqlite"));
		backend = G_TYPE_SQL_CONNNECT_SQLITE;
#endif
#endif
	}
	return backend ;
}
