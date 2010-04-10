/*
 * pcap_hander.h
 *
 *  Created on: 2010-4-9
 *      Author: cai
 */

#ifndef PCAP_HANDER_H_
#define PCAP_HANDER_H_

#include <pcap/pcap.h>
#include <glib.h>

G_BEGIN_DECLS

typedef	gboolean (*pcap_hander_callback)(  struct pcap_pkthdr * ,  const guchar * packet_content, gpointer user_data);

typedef struct _pcap_hander_callback_trunk{
	pcap_hander_callback	func;
	gpointer				user_data;
}pcap_hander_callback_trunk;

gpointer pcap_hander_register_prepend(pcap_hander_callback FUNC, guint16 port,guint16 protocol, gpointer user_data);
gpointer pcap_hander_register(pcap_hander_callback FUNC,guint16 port,guint16 protocol,gpointer user_data);
void pcap_hander_unregister(gpointer hander);
int pcap_hander_get_all(pcap_hander_callback_trunk out[]);
int pcap_hander_get(guint16 port, guint16 protocol,pcap_hander_callback_trunk out[]);

G_END_DECLS

#endif /* PCAP_HANDER_H_ */
