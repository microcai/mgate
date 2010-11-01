/*
 * html_info.c 显示各种信息
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

#include <unistd.h>
#include <sys/resource.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <libsoup/soup.h>
#include "clientmgr.h"
#include "utils.h"
#include "http_server.h"
#include "i18n.h"
#include "global.h"
#include "htmlnode.h"
#include "traffic_status.h"
#include "html_paths.h"

static int cpu_usage;


gboolean celect_usage(gpointer msg)
{
	static struct rusage usage;
	struct rusage cur_usage;

	getrusage(RUSAGE_SELF,&cur_usage);

	cpu_usage = ((cur_usage.ru_utime.tv_sec + cur_usage.ru_stime.tv_sec - usage.ru_utime.tv_sec -usage.ru_stime.tv_sec)*1000

		+ ( cur_usage.ru_utime.tv_usec + cur_usage.ru_stime.tv_usec -  usage.ru_utime.tv_usec - usage.ru_stime.tv_usec)/1000

	)/(100);

	usage = cur_usage;

	return TRUE;
}


void SoupServer_path_info(SoupServer *server, SoupMessage *msg,
		const char *path, GHashTable *query, SoupClientContext *client,
		gpointer user_data)
{
	SoupMessageBody * body = msg->response_body;

	soup_message_set_status(msg,SOUP_STATUS_OK);

	soup_server_pause_message(server,msg);

	soup_message_headers_set_content_type(msg->response_headers,"text/html",NULL);
	soup_message_headers_set_encoding(msg->response_headers,SOUP_ENCODING_CHUNKED);

	HtmlNode * html = htmlnode_new(NULL, "html", NULL);

	gchar * title = g_strdup_printf("Info of the running %s , pid %d",
			PACKAGE_NAME, getpid());

	htmlnode_new_text(htmlnode_new(htmlnode_new_head(html, NULL), "title", NULL), title);

	g_free(title);

	gchar * tr;

	//构造表格吧 :)

	tr = g_strdup_printf("Info of the running %s , pid %d", PACKAGE_NAME,
			getpid());

	HtmlNode * htmlbody = htmlnode_new_body(html, NULL);

	htmlnode_new_text(htmlnode_new(htmlbody, "h1", NULL), tr);

	g_free(tr);

	HtmlNode * div = htmlnode_new(htmlbody, "div", "id=\"info\"", NULL);


	tr = g_strdup_printf("cpu usage %%%d",cpu_usage);

	htmlnode_new_text(htmlnode_new(div, "dd", NULL), tr);

	g_free(tr);

	div = htmlnode_new(htmlbody, "div", "id=\"compiletimeinfo\"", NULL);


	tr = g_strdup_printf("%d",cpu_usage);

	htmlnode_new_text(htmlnode_new(div, "dd", NULL), tr);

	g_free(tr);


	HtmlNode * table = htmlnode_new_table(htmlbody,"align=center",NULL);

	htmlnode_new_text(htmlnode_new(htmlnode_new(table,"tr","align=center",0),"td","align=center",0),"IP状态表");

	gsize num;

	IPStatus * ret = ip_traffic_get_status(&num);

	while(num--)
	{
		HtmlNode * tr = htmlnode_new(table,"tr","align=center",0);

		htmlnode_new_text(htmlnode_new(tr,"td",0),inet_ntoa((struct in_addr){ret[num].ip}));
		htmlnode_new_text_printf(htmlnode_new(tr,"td",0),"下行流量%umb",ret[num].down/(1024*1024));
		htmlnode_new_text_printf(htmlnode_new(tr,"td",0),"下载%ukb/s",ret[num].downspeed/1024);
	}
	g_free(ret);

	htmlnode_to_plane_text_and_free(html,
			(htmlnode_appender) soup_message_body_appender, msg->response_body);

	soup_message_body_complete(body);
	soup_server_unpause_message(server, msg);

}
