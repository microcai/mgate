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

#define title "Log View"


void SoupServer_path_log(SoupServer *server, SoupMessage *msg,
		const char *path, GHashTable *query, SoupClientContext *client,
		gpointer user_data)
{
	SoupMessageBody * body = msg->response_body;

	soup_message_set_status(msg,SOUP_STATUS_OK);

	soup_message_headers_set_content_type(msg->response_headers,"text/html",NULL);
	soup_message_headers_set_encoding(msg->response_headers,SOUP_ENCODING_CONTENT_LENGTH);

	soup_message_body_set_accumulate(body,FALSE);

	HtmlNode * html = htmlnode_new(NULL, "html", NULL);

	HtmlNode * head = htmlnode_new_head(html, NULL);

	htmlnode_new(head,"meta","http-equiv=\"content-type\"","content=\"text/html;charset=utf-8\"",NULL);

	htmlnode_new_text(htmlnode_new(head, "title", NULL), title);

	htmlnode_to_plane_text_and_free(html,
			(htmlnode_appender) soup_message_body_appender, msg->response_body);

	soup_message_body_complete(body);
}
