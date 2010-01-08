/*
 *      main.c
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

#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>

static char socketfile[] = "/tmp/monitor.socket";

int main(int argc, char* argv[])
{
	int sock,buf_size=5;
	struct sockaddr_un addr={0};

	addr.sun_family = AF_LOCAL;
	strncpy(addr.sun_path, socketfile, sizeof(socketfile));
	sock = socket(AF_LOCAL, SOCK_DGRAM, 0);
	sendto(sock, "refresh", buf_size, 0, (struct sockaddr*) &addr, sizeof(addr));
	return (0);
}

