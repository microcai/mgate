/*
 * override.c : this overide some glib/libsoup function to provode bug free release
 *
 *  Created on: 2011-3-19
 *      Author: cai
 */

#include <libsoup/soup.h>


const char     *soup_client_context_get_host        (SoupClientContext *client)
{
	struct sockaddr_in addr;
	socklen_t addrlen = sizeof(addr);

	int socket = soup_socket_get_fd(soup_client_context_get_socket(client));

	getpeername(socket,(struct sockaddr *)&addr,&addrlen);

	return inet_ntoa(addr.sin_addr);
}
