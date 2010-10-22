/*
 * html_path_404.c
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

extern char _binary_404_html_start[];
extern char _binary_404_html_end[];
extern char	_binary_404_html_size[];

void SoupServer_path_404(SoupServer *server, SoupMessage *msg,const char *path,
		GHashTable *query, SoupClientContext *client,gpointer user_data)
{
	soup_message_set_status(msg,SOUP_STATUS_NOT_FOUND);
	soup_message_set_response(msg,"text/html",SOUP_MEMORY_STATIC,_binary_404_html_start,(long)_binary_404_html_size);
}
