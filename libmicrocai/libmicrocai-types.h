/*
 * File:   libmicrocai-types.h
 *
 * type defines for libmicrocai
 *
 *
 */

#ifndef _LIBMICROCAI_TYPES_H
#define	_LIBMICROCAI_TYPES_H

#include <string>
#include <string.h>
#include <time.h>
#include <arpa/inet.h>

typedef char* LPSTR;


enum enum_PROTOL{
	HTTP=80,
 	QQ,
 	MSN
};

enum AccountType{
    Account_QQ = 1000,
    Account_MSN,
    Account_LZ
};

typedef union AccountData{

    u_int32_t QQNumber;
    char*     msn;
    char*     url;
    char*     post;
}AccountData;

namespace parameter_type{

enum parameter_types
{
	STUB,		// just a stub.
	BOOL_short,	// just supply or not to set flag
	BOOL_long,	// must use =yes or =no
	BOOL_both,	// please always use these one
	INTEGER,	// parameter is a integer
	STRING,		// parameter is a string.
	FUNCTION	// parameter is a call back function
};



} // namespace parameter_types

struct parameter_tags{
	const char * const prefix;
	const char * const parameter;
	const char * const discribe;
	const long	 parameter_len;
	const int	 prefix_len;
	const enum	 parameter_type::parameter_types	type;

	parameter_tags(const char*pfix,enum parameter_type::parameter_types tp,const char*p,const long len,const char*dis=NULL):
	prefix(pfix),parameter(p),discribe(dis),parameter_len(len),prefix_len(0),type(tp)
	{
		*(int*)(&prefix_len)  = strlen(this->prefix);
	}
	parameter_tags():
	prefix(0),parameter(0),discribe(0),parameter_len(0),prefix_len(0),type(parameter_type::STUB)
	{}
};

struct so_data
{
    void* module;
    int   nat_helper_event;
    char* config_file;
};


struct Clients_DATA{
    in_addr_t ip;
    struct tm  onlinetime;
    char    MAC_ADDR[6];
    std::string CustomerIDType;
    std::string CustomerName;
    std::string CustomerID;
    std::string RoomNum;
    std::string Floor;
    std::string Build;
    std::string mac_addr;// (xx:xx:xx:xx:xx:xx format)
    std::string ip_addr;
    Clients_DATA()
    {
    	time_t t=time(0);
    	ip=0;
    	onlinetime = *localtime(&t);
    	memset(MAC_ADDR,0,6);
    }
    Clients_DATA(in_addr_t ip)
    {
    	Clients_DATA();
    	ip= ip;

    }
    Clients_DATA(const char* cip)
    {
    	Clients_DATA();
    	ip = inet_addr(cip);
    	ip_addr = cip;
    }
};

typedef int (*PROTOCOL_HANDLER)(struct so_data*,u_char *packet);

typedef int (*pModuleInitFunc)(struct so_data*);

struct handler
{
	u_char magic;
	struct handler * pre,*next;
	int port;//big ending
	int protocol_type;//IPPROTOCOL_TCP or IPPROTOCOL_UDP
	PROTOCOL_HANDLER handler;
};

#endif	/* _LIBMICROCAI_TYPES_H */

