/*
 * html_path_keepalive.c
 *
 *  Created on: 2010-11-1
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
#include <glib/gi18n.h>
#include <libsoup/soup.h>
#include "clientmgr.h"
#include "utils.h"
#include "http_server.h"
#include "global.h"
#include "htmlnode.h"
#include "html_paths.h"
#include "ksql.h"

static void remove_this(Client * client)
{
	g_debug("will remove client %p, id=%s, lastactivetime is %ld",client,client->id,client->last_active_time);

	ksql_vquery_async("delete from roomer_list where ID='%s'",client->id);

}

gboolean remove_outdated_inactive_client(gpointer data)
{
	clientmgr_reomve_outdate_client(GPOINTER_TO_SIZE(data),remove_this);//只有60秒允许啊
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

	if(!client || g_strcmp0(client->id,phone))
		return SoupServer_path_404(server,msg,path,query,soupclient,user_data);

	time(&client->last_active_time);
	soup_message_set_status(msg,SOUP_STATUS_OK);
	soup_message_set_response(msg,"text/html",SOUP_MEMORY_STATIC,keep,sizeof(keep));
}
