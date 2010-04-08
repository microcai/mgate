/*
 * clientmgr.h
 *
 *  Created on: 2010-4-8
 *      Author: cai
 */

#ifndef CLIENTMGR_H_
#define CLIENTMGR_H_

#include <glib-object.h>

G_BEGIN_DECLS

typedef struct _Client{
	GObject	parent;
	GString	* name, * id;
	in_addr_t	ip;
	guchar		mac[6];
}Client;

typedef struct _ClientClass{
	GObjectClass parent_class;
}ClientClass;

//Client Object

GType client_get_type();

//Client Manager

void clientmgr_init();

G_END_DECLS

#endif /* CLIENTMGR_H_ */
