/*
 * 		module.c 模块支持
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
#include <sys/inotify.h>
#include <glob.h>
#include <glib.h>
#include <gmodule.h>
#include <glib/gi18n.h>

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




