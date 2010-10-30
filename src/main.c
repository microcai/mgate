/*
 *      main.c
 *
 *      Copyright 2009-2010 薇菜工作室
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
#include <sys/stat.h>
#include <pcap/pcap.h>
#include <errno.h>
#include <glib.h>
#include <glib/gprintf.h>
#include <fcntl.h>

#include "i18n.h"
#include "global.h"
#include "pcap_thread.h"
#include "clientmgr.h"
#include "module.h"
#include "kpolice.h"
#include "ksql.h"
#include "http_server/http_server.h"

static void myLog(const gchar *log_domain, GLogLevelFlags log_level,
		const gchar *message, gpointer user_data);
static gboolean do_daemon(gpointer user_data);
static void check_pid(gboolean) ;

const gchar * config_file_name = "/etc/mgate.cfg";

static void copyright_notice()
{
	g_printf("%s Version %s\n",PACKAGE_NAME,PACKAGE_VERSION);
	g_printf("Copyright %s %s %d-%d\n","©",_("MicroAppliyLab"),2008,2010);
	g_printf("All rights reserved\n");
}

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
	gchar *  pcapfile = NULL;

#ifdef 	HTTP_SERVER
	gint	httpport = 0 ;
#endif

	gint	thread_num = -1;

	const gchar * module_dir = MODULES_PATH;

	setlocale(LC_ALL,"");
	textdomain(GETTEXT_PACKAGE);
	bindtextdomain(GETTEXT_PACKAGE,GETTEXT_LOCALE_PATH);
#ifdef DEBUG
	bindtextdomain(GETTEXT_PACKAGE,"/tmp//usr/local/share/locale/");
#endif

	g_thread_init(NULL);
	g_type_init();
	g_set_prgname(PACKAGE_NAME);
	g_set_application_name(_("monitor - A monitor tool"));

	GOptionEntry args[] =
	{
			{"daemon",'D',0,G_OPTION_ARG_NONE,&run_daemon,N_("run as daemon")},
			{"flushdb",'f',0,G_OPTION_ARG_NONE,&flush_db,N_("flash the db")},
			{"createdb",'c',0,G_OPTION_ARG_NONE,&createdb,N_("Create database")},
			{"locale",'\0',0,G_OPTION_ARG_STRING,&domain_dir,N_("set domain dir root"),N_("dir")},
			{"config",'f',0,G_OPTION_ARG_STRING,&config_file_name,N_("set alternative config file"),N_("filename")},
			{"module_dir",'f',0,G_OPTION_ARG_STRING,&module_dir,N_("set alternative module dir"),N_("dir")},
			{"device",'d',0,G_OPTION_ARG_STRING,&device,N_("override config, make monitor capturing on that interface"),N_("nic")},
#ifdef 	HTTP_SERVER
			{"http-port",0,0,G_OPTION_ARG_INT,&httpport,N_("override config, make monitor listen on that port"),N_("port")},
#endif
			{"thread-num",0,0,G_OPTION_ARG_INT,&thread_num,N_("override config, make monitor run with num threads"),N_("num")},
			//使用离线的抓包文件而不是在线抓包。调式的时候非常有用
			{"pcapfile",0,0,G_OPTION_ARG_FILENAME,&pcapfile,N_("using dumped file other that live capture. - for stdin"),N_("pcapsavefile")},
			{0}
	};

	time(&t);

	copyright_notice();

	GOptionContext * context;
	context = g_option_context_new("");
	g_option_context_set_ignore_unknown_options(context,TRUE);
	g_option_context_set_translation_domain(context,GETTEXT_PACKAGE);
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
		g_free(device);
	}

	if(thread_num > 0)
		g_key_file_set_integer(gkeyfile,"monitor","threads",thread_num);

	//连接到 mysql
	ksql_init(createdb);

#ifdef HTTP_SERVER

	if(httpport)
	{
		g_key_file_set_integer(gkeyfile,"http","port",httpport);
	}
	//启用内建的 http server
	start_server();
#endif
	//初始化人员管理
	clientmgr_init();

	check_pid(FALSE);

	Kpolice	* police = kpolice_new();

	if(kpolice_check_config(police))
		kpolice_start(police);

	module_enable(module_dir);

	if(pcapfile)
	{
		g_key_file_set_string(gkeyfile,"pcap","pcap",pcapfile);
	}
	g_thread_create((GThreadFunc)pcap_thread_func,police,FALSE,NULL);

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

void myLog(const gchar *log_domain, GLogLevelFlags log_level,
		const gchar *message, gpointer user_data)
{
	const char * level = "info";

	if(log_level & G_LOG_LEVEL_USER_SHIFT)
	{
		level = "user";
	}
	if(log_level & G_LOG_LEVEL_MASK)
	{
		level = "mask";
	}
	if(log_level & G_LOG_LEVEL_INFO)
	{
		level = "info";
	}
	if(log_level & G_LOG_LEVEL_DEBUG)
	{
		level = "DEBUG";
	}
	if(log_level & G_LOG_LEVEL_CRITICAL)
	{
		level = "critical";
	}
	if(log_level & G_LOG_LEVEL_MESSAGE)
	{
		level = "message";
	}
	if(log_level & G_LOG_LEVEL_WARNING)
	{
		level = "warning";
	}
	if(log_level & G_LOG_LEVEL_ERROR)
	{
		level = "error";
	}

	fprintf(user_data,"%s (%d) **%s** : %s\n",PACKAGE_NAME,getpid(),
			level,
			message);
	fflush(user_data);
}


gboolean do_daemon(gpointer user_data)
{
	if(GPOINTER_TO_INT(user_data))
	{
		g_log_set_default_handler(myLog,fopen("/tmp/monitor.log","w"));
		daemon(FALSE,TRUE);
		sleep(0);
	}
	check_pid(TRUE);
	return FALSE;
}
