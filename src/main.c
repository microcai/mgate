/*
 *      main.c
 *
 *      Copyright 2009 MicroCai <microcai@sina.com>
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <sys/signal.h>
#include <sys/syslog.h>
#include <pcap.h>
#include <errno.h>
#include <glib.h>
#include "i18n.h"
#include "global.h"
#include "pcap_thread.h"
#include "clientmgr.h"
#include "module.h"
#include "kpolice.h"
#include "ksql.h"

static void on_term(int p )
{
//	ksql_close();
	exit(0);
}

const gchar * config_file_name = "/etc/monitor.cfg";

int main(int argc, char*argv[], char*env[])
{
	time_t t;
	GMainLoop * loop;
	GError* err = NULL;
	gboolean createdb = FALSE;
	gboolean flush_db = FALSE;
	gboolean run_daemon = FALSE;
	gchar *  domain_dir = NULL;

	const gchar * module_dir = "/usr/lib/monitor/modules" ;

	setlocale(LC_ALL,"");
	textdomain(GETTEXT_PACKAGE);
#ifdef DEBUG
	bindtextdomain(GETTEXT_PACKAGE,"/tmp/share/locale");
#endif

	g_thread_init(NULL);
	g_set_prgname(PACKAGE_NAME);
	g_set_application_name(_("monitor - A monitor tool"));
	g_type_init();

	GOptionEntry args[] =
	{
			{"daemon",'D',0,G_OPTION_ARG_NONE,&run_daemon,_("run as daemon")},
			{"flushdb",'f',0,G_OPTION_ARG_NONE,&flush_db,_("flash the db")},
			{"createdb",'c',0,G_OPTION_ARG_NONE,&createdb,_("Create database")},
			{"locale",'\0',0,G_OPTION_ARG_STRING,&domain_dir,_("set domain dir root"),N_("dir")},
			{"config",'f',0,G_OPTION_ARG_STRING,&config_file_name,_("set alternative config file"),N_("filename")},
			{"module_dir",'f',0,G_OPTION_ARG_STRING,&module_dir,_("set alternative module dir"),N_("dir")},

			{0}
	};

	time(&t);

#ifdef DEBUG
	openlog(PACKAGE_TARNAME,LOG_PERROR|LOG_PID,LOG_USER);
#else
	openlog(PACKAGE_TARNAME,LOG_PID,LOG_USER);
#endif

	syslog(LOG_NOTICE, _("%s loaded at %s"), PACKAGE_NAME,	ctime(&t));

	GOptionContext * context;
	context = g_option_context_new("");
	g_option_context_add_main_entries(context,args,PACKAGE_NAME);
	g_option_context_parse(context,&argc,&argv,NULL);
	g_option_context_free(context);

	if(domain_dir)
	{
		bindtextdomain(GETTEXT_PACKAGE,domain_dir);
		g_free(domain_dir);
	}

	loop = g_main_loop_new(NULL,FALSE);

	umask(0);

	gkeyfile = g_key_file_new();

	if (g_key_file_load_from_file(gkeyfile, config_file_name,
			G_KEY_FILE_KEEP_TRANSLATIONS, NULL))
		syslog(LOG_WARNING, "Err opening config file");

	if(run_daemon)
		run_daemon = daemon(FALSE,FALSE);

	//连接到 mysql
	ksql_init();

	if(createdb)
	{
		ksql_connect_sql();
		ksql_create_db();
	}

	//初始化人员管理
	clientmgr_init();

	kpolice_init();

	module_enable(module_dir);

	g_thread_create((GThreadFunc)pcap_thread_func,NULL,FALSE,NULL);

	signal(15,on_term);
	signal(2,on_term);
	g_main_loop_run(loop);
	return 0;
}

GKeyFile * gkeyfile;
