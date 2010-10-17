/*
 * traffic_status.h
 *
 *  Created on: 2010-10-16
 *      Author: cai
 */

#ifndef TRAFFIC_STATUS_H_
#define TRAFFIC_STATUS_H_

#include <netinet/ip.h>

typedef struct IPStatus{
	in_addr_t	ip;
	guint64		up,down;
	guint		upspeed,downspeed;
	time_t		last_active;
}IPStatus;

G_BEGIN_DECLS

void traffic_status_init();
void traffic_status_end();

void traffic_packet_callback ( in_addr_t ip, in_addr_t mask , struct iphdr * ip_head);

//should g_free results
IPStatus * ip_traffic_get_status(gsize * numberofips);
//remove records of ip
void ip_traffic_reset_ip(in_addr_t ip);
void ip_traffic_reset_all();

G_END_DECLS

#endif /* TRAFFIC_STATUS_H_ */
