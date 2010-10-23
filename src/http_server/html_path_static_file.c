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
#include "unzip.h"
#include "mime.h"

extern const char _binary_resource_zip_start[];
extern const char _binary_resource_zip_end[];

typedef enum inodetype{
	inode_file, //文件
	inode_dirarchive, // zip 压缩的文件夹
}inodetype;

struct inode{
	int			inode_type;
	const char * path;
	const char * content_type;
	gconstpointer	data;
	gconstpointer	data_end;
}inodes[]={
		{ inode_dirarchive , "/" , "stream/unkown",_binary_resource_zip_start,_binary_resource_zip_end},
};

void SoupServer_path_static_file(SoupServer *server, SoupMessage *msg,
		const char *path, GHashTable *query, SoupClientContext *client,
		gpointer user_data)
{
	int i;
	for(i=0;i<G_N_ELEMENTS(inodes);++i)
	{
		switch (inodes[i].inode_type)
		{
			case inode_file:
			if (strcasecmp(inodes[i].path, path) == 0)
			{
				soup_message_set_status(msg, SOUP_STATUS_OK);
				soup_message_set_response(msg, "image/x-icon",
						SOUP_MEMORY_STATIC, inodes[i].data,
						(gsize) inodes[i].data_end - (gsize) inodes[i].data);
				return;
			}
			break;
			case inode_dirarchive:
				if (!strncasecmp(path, inodes[i].path,strlen(inodes[i].path)))
				{
					const char * file = path + strlen(inodes[i].path);
					const zipRecord * ziprec = zipbuffer_search(inodes[i].data,inodes[i].data_end, file);

					if (ziprec)
					{
						const char * zipeddata = (char *)ziprec + sizeof(zipRecord) + ziprec->filename_len + ziprec->extra_len;
						gsize content_length = ziprec->size_ziped;

						soup_message_set_status(msg, SOUP_STATUS_OK);
						soup_message_headers_set_content_type(msg->response_headers,getmime_by_filename(file),0);

						soup_message_headers_set_encoding(msg->response_headers,SOUP_ENCODING_CONTENT_LENGTH);
						soup_message_headers_set_content_length(msg->response_headers,content_length);
						soup_message_headers_replace(msg->response_headers,"Content-Encoding","deflate");

						soup_message_body_append(msg->response_body,SOUP_MEMORY_STATIC,zipeddata,content_length);
						soup_message_body_complete(msg->response_body);
						return ;
					}
				}

				break;
		}
	}
	return SoupServer_path_404(server,msg,path,query,client,user_data);
}
