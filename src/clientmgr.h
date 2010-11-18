/*
 * clientmgr.h -- 客户管理器
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
	gboolean	enable,remove_outdate;
	time_t		last_active_time;
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
Client * client_new(const gchar * name, const gchar * id,const gchar * idtype,guchar mac[6]);

//Client Manager

void clientmgr_init();
Client * clientmgr_get_client_by_ip(in_addr_t ip);
Client * clientmgr_get_client_by_mac(const guchar * mac);
gboolean clientmgr_get_client_is_enable_by_mac(const guchar * mac);
void clientmgr_insert_client_by_mac(guchar * mac,Client * client);
gboolean clientmgr_reomve_client(Client * client);
void clientmgr_reomve_outdate_client(gulong	inactive_time_allowed, void (*removethis)(Client * client) );
G_END_DECLS

#endif /* CLIENTMGR_H_ */
