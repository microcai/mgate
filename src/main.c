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
#include <sys/signal.h>
#include <pcap.h>
#include <errno.h>
#include <glib.h>
#include <fcntl.h>

#include "i18n.h"
#include "global.h"
#include "pcap_thread.h"
#include "clientmgr.h"
#include "module.h"
#include "kpolice.h"
#include "ksql.h"
#include "http_server.h"

static gboolean do_daemon(gpointer user_data);
static void check_pid(gboolean) ;

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
	gchar *	 device = NULL;

	const gchar * module_dir = "/usr/lib/monitor/modules" ;

	setlocale(LC_ALL,"");
	textdomain(GETTEXT_PACKAGE);
#ifdef DEBUG
	bindtextdomain(GETTEXT_PACKAGE,"/tmp/share/locale");
#endif

	g_thread_init(NULL);
	g_type_init();
	g_set_prgname(PACKAGE_NAME);
	g_set_application_name(_("monitor - A monitor tool"));

	GOptionEntry args[] =
	{
			{"daemon",'D',0,G_OPTION_ARG_NONE,&run_daemon,_("run as daemon")},
			{"flushdb",'f',0,G_OPTION_ARG_NONE,&flush_db,_("flash the db")},
			{"createdb",'c',0,G_OPTION_ARG_NONE,&createdb,_("Create database")},
			{"locale",'\0',0,G_OPTION_ARG_STRING,&domain_dir,_("set domain dir root"),N_("dir")},
			{"config",'f',0,G_OPTION_ARG_STRING,&config_file_name,_("set alternative config file"),N_("filename")},
			{"module_dir",'f',0,G_OPTION_ARG_STRING,&module_dir,_("set alternative module dir"),N_("dir")},
			{"device",'d',0,G_OPTION_ARG_STRING,&device,_("override config, make monitor capturing on that interface"),N_("nic")},
			{0}
	};

	time(&t);

	GOptionContext * context;
	context = g_option_context_new("");
	g_option_context_add_main_entries(context,args,PACKAGE_NAME);
	g_option_context_parse(context,&argc,&argv,&err);
	g_option_context_free(context);

	if(err)
	{
		g_error("%s",err->message);
	}

	if(domain_dir)
	{
		bindtextdomain(GETTEXT_PACKAGE,domain_dir);
		g_free(domain_dir);
	}

	loop = g_main_loop_new(NULL,FALSE);

	umask(0);

	gkeyfile = g_key_file_new();

	if (!g_key_file_load_from_file(gkeyfile, config_file_name,
			G_KEY_FILE_KEEP_TRANSLATIONS, NULL))
		g_warning(_("Err opening config file"));

	if(device)
	{
		g_key_file_set_string(gkeyfile,"monitor","nic",device);
	}

	check_pid(FALSE);

	//启用内建的 http server
	start_server();

	//连接到 mysql
	ksql_init(createdb);

	//初始化人员管理
	clientmgr_init();

	kpolice_init();

	module_enable(module_dir);

	g_thread_create((GThreadFunc)pcap_thread_func,NULL,FALSE,NULL);

	signal(15,exit);
	signal(2,exit);
	g_idle_add(do_daemon,GINT_TO_POINTER(run_daemon));
	g_main_loop_run(loop);
	return 0;
}

static void check_pid(gboolean force)
{
	int create;

	int fd = open("/tmp/"PACKAGE_NAME ".pid" , O_NONBLOCK|O_RDWR|O_CLOEXEC );

	if(fd < 0)
	{
		fd = open("/tmp/"PACKAGE_NAME ".pid" , O_CLOEXEC|O_RDWR|O_CREAT,S_IRWXG|S_IRWXU|S_IRWXO);
		create = TRUE;
	}else{
		create = FALSE;
	}

	if(fd < 0)
	{
		g_error(_("unable to create pid file %s : %s"),"/tmp/"PACKAGE_NAME ".pid", g_strerror(errno));
	}

	gchar * buf = g_strdup_printf("%d",getpid());

	if (create)
	{
		write(fd, buf, strlen(buf)+1);
		g_free(buf);
		close(fd);
		return ;
	}
	char buff[21]={0};

	read(fd,buff,20);

	GPid pid = atoi(buff);

	if ( force || (kill(pid,0) && (errno !=EPERM )))
	{
		lseek(fd,0,SEEK_SET);
		write(fd, buf, strlen(buf)+1);
		g_free(buf);
		close(fd);
		return ;
	}else if( pid != getpid())
	{
		g_error(_("pidfile exist, another one is running? pid(%d)"),pid);
	}
}

gboolean do_daemon(gpointer user_data)
{
	if(GPOINTER_TO_INT(user_data))
	{
		daemon(FALSE,TRUE);
		sleep(0);
	}
	check_pid(TRUE);
	return FALSE;
}

GKeyFile * gkeyfile;
