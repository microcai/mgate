/*
 * html_path_static_file.c
 *
 *  Created on: 2010-10-22
 *      Author: cai
 *
 *
 * 这个文件用来向用户返回静态的文件，图片啊，js脚本啊，全静态页面啊，等等
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

extern char	_binary_favicon_ico_start[];
extern size_t _binary_favicon_ico_size;

struct inode{
	const char * path;
	const char * content_type;
	gpointer	data;
	gsize*		data_length;
}inodes[]={
		{ "/favicon.ico" , "image/x-icon",_binary_favicon_ico_start, &_binary_favicon_ico_size },

};

void SoupServer_path_static_file(SoupServer *server, SoupMessage *msg,
		const char *path, GHashTable *query, SoupClientContext *client,
		gpointer user_data)
{
	int i;
	for(i=0;i<G_N_ELEMENTS(inodes);++i)
	{
		if(strcasecmp(inodes[i].path,path)==0)
		{
			soup_message_set_status(msg, SOUP_STATUS_OK);
			soup_message_set_response(msg, "image/x-icon", SOUP_MEMORY_STATIC,
					inodes[i].data, *inodes[i].data_length);
			return ;
		}
	}
	soup_message_set_status(msg, SOUP_STATUS_NOT_FOUND);
}
