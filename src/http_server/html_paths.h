/*
 * html_paths.h
 *
 *  Created on: 2010-10-22
 *      Author: cai
 */

#ifndef HTML_PATHS_H_
#define HTML_PATHS_H_

void soup_message_body_appender(const gchar * txt, SoupMessageBody * body) G_GNUC_INTERNAL;
gboolean celect_usage(gpointer msg) G_GNUC_INTERNAL;

void SoupServer_path_info(SoupServer *_server, SoupMessage *msg,
		const char *path, GHashTable *query, SoupClientContext *client,
		gpointer user_data)  G_GNUC_INTERNAL;

typedef struct{
	GTimer*timer;
	gchar phone[20];
	gchar code[10];
}phonetocode;

gboolean remove_outdated_phone_code_map(gpointer data) G_GNUC_INTERNAL;
void SoupServer_path_getsmscode(SoupServer *_server, SoupMessage *msg,
		const char *path, GHashTable *query, SoupClientContext *client,
		gpointer user_data) G_GNUC_INTERNAL;

void SoupServer_path_login(SoupServer *server, SoupMessage *msg,
		const char *path, GHashTable *query, SoupClientContext *client,
		gpointer user_data) G_GNUC_INTERNAL;

void SoupServer_path_root_icon(SoupServer *server, SoupMessage *msg,
		const char *path, GHashTable *query, SoupClientContext *client,
		gpointer user_data) G_GNUC_INTERNAL;

void SoupServer_path_404(SoupServer *server, SoupMessage *msg,const char *path,
		GHashTable *query, SoupClientContext *client,gpointer user_data)G_GNUC_INTERNAL;

void SoupServer_path_static_file(SoupServer *server, SoupMessage *msg,
		const char *path, GHashTable *query, SoupClientContext *client,
		gpointer user_data) G_GNUC_INTERNAL;

#endif /* HTML_PATHS_H_ */
