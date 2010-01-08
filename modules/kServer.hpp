/*
 * kServer.hpp
 *
 *  Created on: 2009-5-11
 *      Author: cai
 *
 * Definition for clients to communicate with server
 *
 */

#ifndef KSERVER_HPP_
#define KSERVER_HPP_

#define SERVER_PORT 21314 // "SB" == 21314

#ifdef  	 SERVER_SIDE

bool operator  < (const struct sockaddr_in & __x,
		const struct sockaddr_in & __y)
{
	return ((u_int64_t) __x.sin_port + (u_int64_t) __x.sin_addr.s_addr)
			< ((u_int64_t) __y.sin_port + (u_int64_t) __y.sin_addr.s_addr);
}
#endif /*SERVER_SIDE*/

#define SYN_WAIT	0
#define SYN_ESTABLISHED	1
#define SYN_WAIT_ACK	2
#define SYN_STATE_OK	3
#define SYN_WAIT_ACK_TIMEDOUT		4

#define MODE_NEED_PASSWD 0
#define MODE_OK		     1

#define MODE_LOGINED	2
#define MODE_NEED_LOGIN 0






#ifdef  	 SERVER_SIDE

struct active_client{
	ulong login_code;
	u_int64_t current_code;
	u_int64_t prev_code;
	time_t last_active_time;
	sockaddr_in sock_addr;
	u_int64_t passwd_token;
	int mode;
	int	state; // 模拟  TCP
	int lastdata_len;
	char * lastdata;
	~active_client()
	{
		if(lastdata)
			delete [] lastdata;
	}
};

struct writedata{
	sockaddr_in	addr;
	size_t	length;
	char	data[ETHERMTU];
};


#endif /*SERVER_SIDE*/




#endif /* KSERVER_HPP_ */
