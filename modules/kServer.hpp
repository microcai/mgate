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

bool operator  < (const struct sockaddr_in & __x,
		const struct sockaddr_in & __y)
{
	return ((u_int64_t) __x.sin_port + (u_int64_t) __x.sin_addr.s_addr)
			< ((u_int64_t) __y.sin_port + (u_int64_t) __y.sin_addr.s_addr);
}

#pragma push
#pragma pack(1)


#pragma pop


#ifdef  	 SERVER_SIDE

struct active_client{
	u_int64_t login_code;
	u_int64_t current_code;
	u_int64_t prev_code;
	time_t last_active_time;
	sockaddr_in sock_addr;
	u_int64_t passwd_token;
};




#endif /*SERVER_SIDE*/
#endif /* KSERVER_HPP_ */
