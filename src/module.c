/*
 * load_module.cpp
 *
 * Copyright 2009 microcai <microcai@sina.com>
 *
 * See COPYING for more details about this software's license
 */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <sys/inotify.h>
#include <glob.h>
#include <glib.h>
#include <gmodule.h>

#include "i18n.h"

gboolean module_change(GIOChannel *source, GIOCondition condition,gpointer data)
{
	//目前只是 discard change notify
	char buf[200];
	gsize	read;
	g_io_channel_read(source,buf,sizeof(buf),&read);
	return TRUE;
}

static void modlue_load_so(const gchar * module)
{
	g_message("load module %s",module);
	GModule * m = g_module_open(module,G_MODULE_BIND_LOCAL);
	if(!m)
		g_warning("%s",g_module_error());
}


static gboolean module_real_load(gpointer user_data)
{
	const gchar * dir = (typeof(dir))user_data;

	int i ;

	glob_t globs = {0};

	gchar * glob_pattern = g_strdup_printf("%s/*.so",dir);

	glob(glob_pattern,GLOB_DOOFFS,NULL,&globs);

	for(i=0;i < globs.gl_pathc; i ++)
	{
		gchar * filename = globs.gl_pathv[i];
		modlue_load_so(filename);
	}

	globfree(&globs);

	return FALSE;

	int fd = inotify_init1(IN_CLOEXEC);
	GIOChannel * chl = g_io_channel_unix_new(fd);

	inotify_add_watch(fd,dir,0);

	g_io_add_watch(chl,G_IO_IN,module_change,0);

	return FALSE;
}

void module_enable(const gchar * module_dir)
{
	module_real_load((gpointer)module_dir);
}




