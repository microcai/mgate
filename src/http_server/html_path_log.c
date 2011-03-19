/*
 * html_log.c 显示上网日志
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
#include <glib/gi18n.h>
#include <libsoup/soup.h>
#include "clientmgr.h"
#include "utils.h"
#include "http_server.h"
#include "global.h"
#include "htmlnode.h"
#include "traffic_status.h"
#include "html_paths.h"
#include "gsqlconnect.h"

#define soup_message_body_append_static(body,txt) \
	soup_message_body_append(body,SOUP_MEMORY_STATIC,txt,sizeof(txt)-1)

typedef struct idle_param{
	SoupServer *server;
	SoupMessage *msg;
	const char *path;
	gint	page;
	SoupClientContext *client;
	GSQLConnect * connector;
	SoupMessageBody * body;
	gpointer user_data;
	gint		stage;
}idle_param;


/* 空闲时调用*/
static gboolean idle_do_connect(idle_param * ptr)
{
	gchar * sql ;
	GSQLResult	* result;
	gchar * line;

	switch(ptr->stage++)
	{
	case 0 : // 连接数据库
		if(! g_sql_connect_real_connect(ptr->connector,NULL) )
		{
			// 失败
			// 数据库连接失败描述

			ptr->stage = -1;
		}
	break;
	case 1: //构建 SQL 语句吧
		// 获得 页面参数, 每页显示 100 个。
		sql = g_strdup_printf("select * from t_netlog	order by nIndex desc  limit %d,100",ptr->page*100);

		if(!g_sql_connect_run_query(ptr->connector,sql,-1))
			ptr->stage = -1;
		g_free(sql);

		//上一页，下一页
		soup_message_body_append_static(ptr->body,"\t<div id=\"Header\">");
		soup_message_body_append_static(ptr->body,"<a href=\"/log\">首页</a>");

		if( ptr->page ==0 )
			line = g_strdup_printf("<a href=\"/log?page=%d\">下一页</a>",ptr->page+1);
		else if ( ptr->page >= 1)
		{
			line = g_strdup_printf("<a href=\"/log?page=%d\">上一页</a><a href=\"/log?page=%d\">下一页</a>",ptr->page-1,ptr->page+1);
		}

		soup_message_body_append(ptr->body,SOUP_MEMORY_COPY,line,strlen(line));

		soup_message_body_append_static(ptr->body,"</div>\n");

		break;
	case 2:
		// 开始获得每行代码。
		result = g_sql_connect_use_result(ptr->connector);
		if(result && g_sql_result_fetch_row(result))
		{
			soup_message_body_append_static(ptr->body,"\t<div id=\"PageBody\">");

			line = g_strdup_printf(
					"<span id=\"nIndex\">%s</span> <span id=\"Cname\">%s</span><span id=\"URL\"><p>%s</p></span>",
					g_sql_result_colum_by_name(result,"nIndex"),
					g_sql_result_colum_by_name(result,"CustomerName"),
					g_sql_result_colum_by_name(result,"strLogInfo")
					);

			soup_message_body_append(ptr->body,SOUP_MEMORY_COPY,line,strlen(line));

			soup_message_body_append_static(ptr->body,"</div>\n");

			ptr->stage = 2;
		}
		break;
	case -1: //连接失败，
	default: //完毕
		if(ptr->stage == -1)
		{
			//向用户说明数据连接失败。
		}

		soup_message_body_append_static(ptr->body,"</div>\n</body></html>");
		soup_server_unpause_message(ptr->server,ptr->msg);
		soup_message_body_complete(ptr->msg->response_body);
		if(ptr->connector)
			g_object_unref(ptr->connector);
		return FALSE; //空闲过程取消
	}
	return TRUE;//继续空闲回调
}

struct SoupClientContext {
	SoupServer     *server;
	SoupSocket     *sock;
	SoupAuthDomain *auth_domain;
	char           *auth_user;

	int             ref_count;
};


// 连接到数据库进行查询。
void SoupServer_path_log(SoupServer *server, SoupMessage *msg,
		const char *path, GHashTable *query, SoupClientContext *client,
		gpointer user_data)
{
	GSQLConnect * connector;

	SoupMessageBody * body = msg->response_body;

	soup_message_set_status(msg,SOUP_STATUS_OK);

	soup_message_headers_set_content_type(msg->response_headers,"text/html",NULL);
	soup_message_headers_set_encoding(msg->response_headers,SOUP_ENCODING_CHUNKED);

	soup_message_body_set_accumulate(body,FALSE);

	soup_message_body_append_static(body,"<html><head>\n\t<meta http-equiv=\"content-type\" content=\"text/html;charset=utf-8\" />\n<title>网络日志</title>\n");

	soup_message_body_append_static(body,"\t<link href=\"css.css\" rel=\"stylesheet\" type=\"text/css\" />\n</head>\n");

	soup_message_body_append_static(body,"<div id=\"container\">");


	connector = (GSQLConnect*)g_object_new(sqlconnect_get_backend(),NULL);

	idle_param * param = g_new0(idle_param,1);

	param->server = server; param->connector = connector;
	param->msg = msg; param->user_data = user_data;
	param->path = path ; param->client = client; param->body = body;

	if(g_hash_table_lookup(query,"page"))
		param->page = g_ascii_strtoll(g_hash_table_lookup(query,"page"),NULL,10);

	/*
	 * IDLE 时操作数据库.这是为了避免长时间的数据库查询让
	 * HTTP服务器壅塞导致其他用户无法访问。因为本HTTP服务器是单线程运作的。
	 */
	if(connector && g_sql_connect_check_config(connector))
	{
		param->stage = 0;
	}else //数据库连接失败
	{
		param->stage = -1;
	}
	g_idle_add_full(G_PRIORITY_LOW,(GSourceFunc)idle_do_connect,param,g_free);
	soup_server_pause_message(server,msg);
}
