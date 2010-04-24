/*
 * clientmgr.h
 *
 *  Created on: 2010-4-8
 *      Author: cai
 */

#ifndef CLIENTMGR_H_
#define CLIENTMGR_H_

#include <netinet/in.h>

#include <glib-object.h>

G_BEGIN_DECLS

typedef struct _Client{
	GObject	parent;
	const gchar * name, * id,*idtype,*room;
	in_addr_t	ip;
	guchar		mac[6];
	gboolean	enable;
}Client;

typedef struct _ClientClass{
	GObjectClass parent_class;
}ClientClass;

//Client Object
#define G_TYPE_CLIENT	(client_get_type())
#define CLIENT(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), G_TYPE_CLIENT, Client))
#define CLIENT_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), G_TYPE_CLIENT, ClientClass))
#define IS_CLIENT(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), G_TYPE_CLIENT))
#define IS_CLIENT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), G_TYPE_CLIENT))
#define CLIENT_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), G_TYPE_CLIENT, ClientClass))


GType client_get_type() G_GNUC_CONST;
Client * client_new(const gchar * name, const gchar * id,const gchar * idtype);

//Client Manager

void clientmgr_init();
Client * clientmgr_get_client_by_ip(in_addr_t ip);
Client * clientmgr_get_client_by_mac(const guchar * mac);
void clientmgr_insert_client_by_mac(guchar * mac,Client * client);


G_END_DECLS

#endif /* CLIENTMGR_H_ */
