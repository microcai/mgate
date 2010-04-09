/*
 * pcap_hander.h
 *
 *  Created on: 2010-4-9
 *      Author: cai
 */

#ifndef PCAP_HANDER_H_
#define PCAP_HANDER_H_

#include <glib.h>

G_BEGIN_DECLS

typedef	gboolean (*pcap_hander_callback)();


gpointer pcap_hander_register(pcap_hander_callback FUNC,guint16 port,guint16 protocol,gpointer user_data);

G_END_DECLS

#endif /* PCAP_HANDER_H_ */
