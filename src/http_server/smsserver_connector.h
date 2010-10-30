/*
 * smsserver_connector.h -- 短信服务器客户端
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

#ifndef SMSSERVER_CONNECTOR_H_
#define SMSSERVER_CONNECTOR_H_

typedef struct smsserver_result{
	int statuscode;
	const char * buffer; // for give back message , or for client to send message, if send, we will free it with g_free.
	gssize		 length;
} smsserver_result;
typedef void (*smsserver_readycallback)
		(smsserver_result*, SoupMessage *msg,const char *path,GHashTable *query, SoupClientContext *client,gpointer user_data);

/*读取配置文件，并开始检测短信服务器是否在线*/
void smsserver_pinger_start();

/*判断短信服务器是否在线*/
gboolean smsserver_is_online();

/*到服务器获取验证码*/
void smsserver_getcode(smsserver_readycallback usercb,SoupMessage * msg,const char *path,GHashTable *query, SoupClientContext *client,gpointer user_data);

/*获取使用此验证码的手机号码*/
void smsserver_verifycode(smsserver_readycallback usercb,const char * code,SoupMessage * msg,const char *path,GHashTable *query, SoupClientContext *client,gpointer user_data);

#endif /* SMSSERVER_CONNECTOR_H_ */
