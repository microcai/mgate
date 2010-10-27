/*
 * html_path_login.c
 *
 *  Created on: 2010-10-22
 *      Author: cai
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
#include "html_paths.h"
#include "smsserver_connector.h"

gboolean remove_outdated_inactive_client(gpointer data)
{
	clientmgr_reomve_outdate_client(GPOINTER_TO_SIZE(data));//只有60秒允许啊
	return TRUE;
}

//登录完成后的页面将不停的刷新本页面(每15s)，如果他在一分钟内没有刷新，那么就判断为下线。
void SoupServer_path_keep_alive(SoupServer *server, SoupMessage *msg,const char *path,
		GHashTable *query, SoupClientContext *soupclient,gpointer user_data)
{
	static char keep[]=""
			"<html>\n\t<head>\n\t\t<meta http-equiv=\"Refresh\"content=\"5\">\n"
			"\t</head>\n</html>\n";

	//使用 GET 的方法啦。
	// GET /keep_alive?phone=xxxxxxx
	const char * phone = g_hash_table_lookup(query,"phone");

	const gchar * ip = soup_client_context_get_host(soupclient);

	Client * client = clientmgr_get_client_by_ip(inet_addr(ip));

	if(client && !g_strcmp0(client->id,phone))
		return SoupServer_path_404(server,msg,path,query,soupclient,user_data);

	time(&client->last_active_time);
	soup_message_set_status(msg,SOUP_STATUS_OK);
	soup_message_set_response(msg,"text/html",SOUP_MEMORY_STATIC,keep,sizeof(keep));
}

void SoupServer_path_login(SoupServer *server, SoupMessage *msg,const char *path,
		GHashTable *query, SoupClientContext *client,gpointer user_data)
{
	guchar mac[6];
	const gchar * ip = soup_client_context_get_host(client);

	int sockclient = soup_socket_get_fd(soup_client_context_get_socket(client));

	if (g_strcmp0(msg->method, "POST") == 0 )
	{
		soup_message_set_status(msg, SOUP_STATUS_OK);

		soup_message_headers_set_content_type(msg->response_headers, "text/html; charset=UTF-8",
				NULL);
		soup_message_headers_set_encoding(msg->response_headers,
				SOUP_ENCODING_CHUNKED);

		HtmlNode * html = htmlnode_new(NULL,"html",NULL);

		char phone[32] =
		{ 0 };

		if (smsserver_is_online())
			sscanf(msg->request_body->data, "phone=%31[0123456789]", phone);
		else
			strcpy(phone,ip);

		HtmlNode *head =  htmlnode_new_head(html,NULL);

		htmlnode_new_jssrc(head,"jquery-1.4.3.js");
		htmlnode_new_jssrc(head,"jheartbeat.js");

		HtmlNode * body = htmlnode_new_body(html,NULL);

		HtmlNode * p = htmlnode_new(body,"p",NULL);

		if(arp_ip2mac(inet_addr(ip),mac,sockclient))
		{
			Client * client = client_new(phone,phone,"990",mac);
			g_object_set(client,"ipstr", ip, "enable",TRUE,NULL);

			//TODO ; check if it is self owned computer
			client->remove_outdate = TRUE;

			clientmgr_insert_client_by_mac(mac,client);

			HtmlNode * div = htmlnode_new(body,"div","id=\"test_div\"",0);

			p = htmlnode_new(div,"p",NULL);

			if(smsserver_is_online())
				htmlnode_new_text(p,"手机号:");
			else
				htmlnode_new_text(p,"IP地址:");
			htmlnode_new_text(p,phone);
			htmlnode_new_text(p,"登录成功，你现在起可以自由访问网络了:)");
			htmlnode_new_text(htmlnode_new(div,"p",NULL),"请不要关闭本页。如果您关闭了本页面，您将立即断网");
			htmlnode_new_text(htmlnode_new(htmlnode_new_head(html,NULL),"title",NULL),"登录成功!");

			gchar * keep_aliveurl = g_strdup_printf("/keep_alive?phone=%s",phone);

			htmlnode_new_iframe(body,keep_aliveurl,"height=\"-1\"","width=\"-1\"",0);

			g_free(keep_aliveurl);

		}else
		{
			htmlnode_new_text(htmlnode_new(htmlnode_new_head(html,NULL),"title",NULL),"登录失败!");
			htmlnode_new_text(p,"登录失败，失败啊!你的 手机 是 ");
			htmlnode_new_text(p,phone);
		}
		htmlnode_to_plane_text_and_free(html,(htmlnode_appender)soup_message_body_appender,msg->response_body);
		soup_message_body_complete(msg->response_body);
	}
}
