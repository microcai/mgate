/*
 * html_path_static_file.c  --
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
#include <sys/mman.h>
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

void SoupServer_path_static_file(SoupServer *server, SoupMessage *msg,
		const char *path, GHashTable *query, SoupClientContext *client,
		gpointer user_data)
{
	//首先到 overlay 里找
	gpointer filecontent;
	gsize filelength;

	const zipRecord * ziprec;

	soup_message_set_status(msg, SOUP_STATUS_OK);
	soup_message_headers_set_content_type(msg->response_headers,getmime_by_filename(path),0);
	soup_message_headers_set_encoding(msg->response_headers,SOUP_ENCODING_CONTENT_LENGTH);

	if ( overlay_get_file(path, user_data, &filecontent, &filelength))
	{
		//返回文件内容
		soup_message_headers_set_content_length(msg->response_headers,filelength);
		soup_message_body_append(msg->response_body,SOUP_MEMORY_COPY,filecontent,filelength);
		//释放文件
		munmap(filecontent,filelength);
	}else if ((ziprec = static_file_get_zip(path)))
	{
		const char * zipeddata = (char *)ziprec + sizeof(zipRecord) + ziprec->filename_len + ziprec->extra_len;
		gsize content_length = ziprec->size_ziped;

		soup_message_headers_set_content_length(msg->response_headers,content_length);
		soup_message_headers_replace(msg->response_headers,"Content-Encoding","deflate");
		soup_message_body_append(msg->response_body,SOUP_MEMORY_STATIC,zipeddata,content_length);
	}else
		return SoupServer_path_404(server,msg,path,query,client,user_data);

	soup_message_body_complete(msg->response_body);
}
