/*
 * File:   libmicrocai-functions.h
 * Author: cai
 *
 * Created on 2009年4月15日, 上午10:24
 */

#ifndef _LIBMICROCAI_FUNCTIONS_H
#define	_LIBMICROCAI_FUNCTIONS_H

#include "libmicrocai-macros.h"

_EXTERN_ void ParseParameters(int * argc, char ** argv[], struct parameter_tags p_[]);

_EXTERN_ std::string GetToken(const char* strings,const char * key);

_EXTERN_ void run_cmd(const std::string & strcmd );
_EXTERN_ int Wait_For_event(int __event,int timedout);

_EXTERN_ void Wake_all(int __event);

_EXTERN_ bool
	GetMac(const char *ip, char MAC_ADDR[],char mac_addr[]);

_EXTERN_ int
	enum_and_load_modules(const char*,struct so_data *);

_EXTERN_    void*
    register_protocol_handler(PROTOCOL_HANDLER, int,int IPPROTOCOL_TYPE);

_EXTERN_    int
    un_register_protocol_handler(void*p);

_EXTERN_    PROTOCOL_HANDLER*
    get_registerd_handler(int port,int IPPROTOCOL_TYPE);

_EXTERN_ u_int16_t
    checksum(u_int16_t *buffer, int size);
_EXTERN_ int
    is_client_online(in_addr_t ip);
_EXTERN_ int
    set_client_offline(in_addr_t ip);

_EXTERN_ void
	set_client_online(in_addr_t ip,struct Clients_DATA* data) throw();
_EXTERN_ struct Clients_DATA*
	get_client_data(in_addr_t ip);

#endif	/* _LIBMICROCAI_FUNCTIONS_H */

