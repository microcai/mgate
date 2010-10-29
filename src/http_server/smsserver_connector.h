/*
 * smsserver_connector.h
 *
 *  Created on: 2010-10-27
 *      Author: cai
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

void smsserver_pinger_start();
gboolean smsserver_is_online();
void smsserver_getcode(smsserver_readycallback usercb,SoupMessage * msg,const char *path,GHashTable *query, SoupClientContext *client,gpointer user_data);

#endif /* SMSSERVER_CONNECTOR_H_ */
