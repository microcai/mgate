/*
 * html_resource.c  --
 * 	这个文件用来向用户返回静态的文件，图片啊，js脚本啊，全静态页面啊，等等
 *
 *      Copyright 2010-2011 薇菜工作室
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
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <unistd.h>
#include <sys/resource.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <libsoup/soup.h>
#include "clientmgr.h"
#include "utils.h"
#include "http_server.h"
#include "global.h"
#include "htmlnode.h"
#include "traffic_status.h"
#include "html_paths.h"
#include "unzip.h"
#include "mime.h"

extern const char _binary_resource_zip_start[];
extern const char _binary_resource_zip_end[];

const zipRecord * static_file_get_zip(const char * path)
{
	const char * file = path + 1;
	return zipbuffer_search(_binary_resource_zip_start,_binary_resource_zip_end, file);
}
