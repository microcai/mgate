
/*
 *      protocol_def.h
 *
 *      Copyright 2009 MicroCai <microcai@sina.com>
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
#ifndef __________POOTOCOL_DEF__H
#define __________POOTOCOL_DEF__H 1
#include <netinet/ip.h>
#include <stdio.h>
#include <pcap.h>

struct pcap_thread_args{
	in_addr_t ip;
	in_addr_t mask;
	pcap_t	* pcap_handle;
	u_char	  mac_addr[6];
};
_EXTERN_ void *pcap_thread_func( struct pcap_thread_args * );
int Check_update(void *);

#endif
