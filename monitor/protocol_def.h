
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

struct pcap_thread_args{
	in_addr_t ip;
	in_addr_t mask;
	char eth[8];
	char bpf_filter_string[200];

};

static inline int stob ( char*s )
{
	if ( strncmp ( s,"yes",3 ) )
		return 0;
	else
		return 1;
}


void redirect_to_local_http( u_int32_t ,const u_char *,struct iphdr* );

#endif
