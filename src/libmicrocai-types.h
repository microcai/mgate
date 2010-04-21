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

