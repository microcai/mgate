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

enum LOG_PRINT_LEVEL{
	L_OUTPUT,
	L_FAITAL,
	L_ERROR,
	L_WARNING,
	L_NOTICE,
	L_DEBUG_OUTPUT_MORE,
	L_DEBUG_OUTPUT
};
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
};


class Clients_DATA{
public:
    in_addr_t ip;
    struct tm  logintime;
    struct tm  onlinetime;
    u_char    MAC_ADDR[6];
    std::string CustomerIDType;
    std::string CustomerName;
    std::string CustomerID;
    std::string RoomNum;
    std::string Floor;
    std::string Build;
    std::string mac_addr;// (xx:xx:xx:xx:xx:xx format)
    std::string ip_addr;
    int nIndex;

    Clients_DATA();
    ~Clients_DATA();

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
template< typename ptr_type >
class auto_ptr{
private:
	ptr_type* m_autoptr;
public:
	auto_ptr(ptr_type* obj)
	{
		m_autoptr = obj;
	}
	~auto_ptr(){
		delete m_autoptr;
	}
	operator ptr_type * ()
	{
		return m_autoptr;
	}
};

class auto_str{
private:
	char * m_autoptr;
public:
	auto_str(char* obj)
	{
		m_autoptr = obj;
	}
	auto_str(size_t sz){
		m_autoptr = new char[sz];
	}
	~auto_str(){
		delete[] m_autoptr;
	}
	operator char * ()
	{
		return m_autoptr;
	}
};


#ifdef _______DDDDD
struct tm
{
  int tm_sec;			/* Seconds.	[0-60] (1 leap second) */
  int tm_min;			/* Minutes.	[0-59] */
  int tm_hour;			/* Hours.	[0-23] */
  int tm_mday;			/* Day.		[1-31] */
  int tm_mon;			/* Month.	[0-11] */
  int tm_year;			/* Year	- 1900.  */
  int tm_wday;			/* Day of week.	[0-6] */
  int tm_yday;			/* Days in year.[0-365]	*/
  int tm_isdst;			/* DST.		[-1/0/1]*/

  long int __tm_gmtoff;		/* Seconds east of UTC.  */
  __const char *__tm_zone;	/* Timezone abbreviation.  */

};

#endif

#endif	/* _LIBMICROCAI_TYPES_H */

