/*
 *      redirect_to_local_http.h
 *
 *      Copyright 2009 microcai <microcai@sina.com>
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

#ifndef redirect_to_local_http_h
#define redirect_to_local_http_h

u_int8_t httphead[512];

u_int8_t httphead_t[] =
"HTTP/1.0 302 Found\n\rLocation: http://%s\n\rConnection: close\n\r\n\r<head><meta http-equiv=\"Refresh\"content=\"0 ; url=http://%s\"></head></html>";

extern char net_interface[];

#endif
