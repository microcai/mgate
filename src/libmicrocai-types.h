/*
 * File:   libmicrocai-types.h
 *
 * type defines for libmicrocai
 *
 *
 */

#ifndef _LIBMICROCAI_TYPES_H
#define	_LIBMICROCAI_TYPES_H

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


struct so_data
{
    void* module;
};


typedef struct Clients_DATA
{
	in_addr_t ip;
	struct tm logintime;
	struct tm onlinetime;
	u_char MAC_ADDR[6];
	gchar CustomerIDType[32];
	char CustomerName[32];
	char CustomerID[32];
	char RoomNum[32];
	char Floor[32];
	char Build[32];
	char mac_addr[32];// (xx:xx:xx:xx:xx:xx format)
	char ip_addr[32];
	int nIndex;
}Clients_DATA;

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

typedef void ** KSQL_RES;
typedef char ** KSQL_ROW;


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

