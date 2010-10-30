/*
 * traffic_status.h
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
