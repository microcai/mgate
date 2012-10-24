/*
 * utils.h
 *
 *      Copyright 2010 薇菜工作室
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

#ifndef UTILS_H_
#define UTILS_H_

#include <glib.h>

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


void  convertMAC(guchar mac[6],const char * strmac);
void formatMAC(const u_char * MAC_ADDR,char * strmac);
guint64	mac2uint64( guchar mac[6]);

int gbk_utf8(char *outbuf, size_t outlen, const char *inbuf, size_t inlen);
int utf8_gbk(char *outbuf, size_t outlen, const char *inbuf, size_t inlen);
struct tm * GetCurrentTime();
double GetDBTime_str(char *pTime);
double GetDBTime_tm(struct tm * ptm);

gboolean arp_ip2mac(in_addr_t ip, guchar mac[6],int sock);

//判断身份证号码有效性
gboolean verify_id(char * idnum);

G_END_DECLS


#endif /* UTILS_H_ */
