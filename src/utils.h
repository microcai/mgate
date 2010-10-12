/*
 * utils.h
 *
 *  Created on: 2010-4-8
 *      Author: cai
 */

#ifndef UTILS_H_
#define UTILS_H_

#include <glib.h>

#include "kpolice.h"

G_BEGIN_DECLS

typedef enum NetAcountType{
    NetAcountType_MAC_ADDR=1000,
    NetAcountType_MSN = 1001,
    NetAcountType_QQ = 1002,
    NetAcountType_HTTP = 1003,
    NetAcountType_POST = 1004,
    NetAcountType_LZ   = 1005
}NetAcountType;

struct NetAcount{
    NetAcountType type;
    char strType[8];
    in_addr_t   ip;
    in_addr_t   dstip;
    char*	packet;
    char*    host;
    char* passwd;
    char* data;
    u_short dport;
//    NetAcount(enum NetAcountType type,const u_char * _packet)
//    {
//    	ip = dstip = 0;
//    	memset(strType,0,8);
//    	host  = "0" ;
//    	passwd = "";
//    	data = "0";
//    	packet = (char*) _packet;
//    }
};


void  convertMAC(char mac[6],const char * strmac);
void formatMAC(const u_char * MAC_ADDR,char * strmac);
guint64	mac2uint64( guchar mac[6]);

int gbk_utf8(char *outbuf, size_t outlen, const char *inbuf, size_t inlen);
int utf8_gbk(char *outbuf, size_t outlen, const char *inbuf, size_t inlen);
struct tm * GetCurrentTime();
double GetDBTime_str(char *pTime);
double GetDBTime_tm(struct tm * ptm);

void RecordAccout(const char * type,in_addr_t ip,in_addr_t destip, const char mac[6], const char * host , const char * passwd,const void * data, unsigned short dport,Kpolice *);


//判断身份证号码有效性
gboolean verify_id(char * idnum);

G_END_DECLS


#endif /* UTILS_H_ */
