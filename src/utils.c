/*
 * utils.c 工具集合
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#include <net/if_arp.h>
#include <net/ethernet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <glib.h>
#include "i18n.h"
#include "utils.h"
#include "kpolice.h"
#include "ksql.h"
#include "global.h"
#include "clientmgr.h"


static inline char hex2char(const char str[])
{
	u_char byte1, byte2;
	if (str[0] >= 'a')
		byte1 = str[0] - 'a' + 10;
	else if (str[0] >= 'A')
		byte1 = str[0] - 'A' + 10;
	else
		byte1 = str[0] - '0';

	if (str[1] >= 'a')
		byte2 = str[1] - 'a' + 10;
	else if (str[1] >= 'A')
		byte2 = str[1] - 'A' + 10;
	else
		byte2 = str[1] - '0';
	u_char ret = byte1 << 4 | byte2;

	return *(char*) (&ret);
}

void  convertMAC(guchar mac[6],const char * strmac)
{
	int i;
	for (i = 0; i < 6; ++i)
	{
		mac[i] = hex2char(& strmac[i * 3] );
	}
}

void formatMAC(const u_char * MAC_ADDR,char * strmac)
{
	sprintf(strmac, "%02x:%02x:%02x:%02x:%02x:%02x",
			MAC_ADDR[0], MAC_ADDR[1], MAC_ADDR[2], MAC_ADDR[3], MAC_ADDR[4], MAC_ADDR[5]);

}


guint64	mac2uint64( guchar mac[6])
{
	guint64	tmac=0;
	memcpy(&tmac,mac,6);
	return tmac;//(guint64)mac[0] + ((guint64)mac[1]<<8) + ((guint64)mac[2] <<16) +	((guint64)mac[3]<<24) + ((guint64)mac[4]<<32) + ((guint64)mac[5] << 40);
}

struct tm * GetCurrentTime()
{
	time_t t;
	time(&t);
	return localtime(&t);
}

double GetDBTime_tm(struct tm * ptm)
{
	u_short wYear = ptm->tm_year + 1900;
	u_short wMonth = ptm->tm_mon + 1;
	u_short wDay = ptm->tm_mday;
	u_short wHour = ptm->tm_hour;
	u_short wMinute = ptm->tm_min;
	u_short wSecond = ptm->tm_sec;
	int _afxMonthDays[13] =
	{ 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 };

	gboolean bLeapYear = ((wYear & 3) == 0) && ((wYear % 100) != 0 || (wYear % 400)
			== 0);
	long nDate = wYear * 365L + wYear / 4 - wYear / 100 + wYear / 400
			+ _afxMonthDays[wMonth - 1] + wDay;

	if (wMonth <= 2 && bLeapYear)
		--nDate;
	nDate -= 693959L;

	double dblTime = (((long) wHour * 3600L) + ((long) wMinute * 60L)
			+ ((long) wSecond)) / 86400.;

	double dbTime = (double) nDate + ((nDate >= 0) ? dblTime : -dblTime);
	return dbTime;
}

double GetDBTime_str(char *pTime)
{
	int nYear = 0, nMon = 0, nDay = 0, nHour = 0, nMin = 0, nSec = 0;
	char *p = strstr(pTime, "-");
	if (!p)
		return 0;
	char strTemp[8];
	memset(strTemp, 0, 8);
	strncpy(strTemp, pTime, p - pTime);
	nYear = atoi(strTemp);
	pTime = p + 1;

	p = strstr(pTime, "-");
	if (!p)
		return 0;
	memset(strTemp, 0, 8);
	strncpy(strTemp, pTime, p - pTime);
	nMon = atoi(strTemp);
	pTime = p + 1;

	p = strstr(pTime, " ");
	if (!p)
		return 0;
	memset(strTemp, 0, 8);
	strncpy(strTemp, pTime, p - pTime);
	nDay = atoi(strTemp);
	pTime = p + 1;

	p = strstr(pTime, ":");
	if (!p)
		return 0;
	memset(strTemp, 0, 8);
	strncpy(strTemp, pTime, p - pTime);
	nHour = atoi(strTemp);
	pTime = p + 1;

	p = strstr(pTime, ":");
	if (!p)
		return 0;
	memset(strTemp, 0, 8);
	strncpy(strTemp, pTime, p - pTime);
	nMin = atoi(strTemp);
	pTime = p + 1;
	if (!pTime)
		return 0;

	nSec = atoi(pTime);

	int _afxMonthDays[13] =
	{ 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 };

	unsigned short int wYear = nYear;
	unsigned short int wMonth = nMon;
	unsigned short int wDay = nDay;
	unsigned short int wHour = nHour;
	unsigned short int wMinute = nMin;
	unsigned short int wSecond = nSec;

	//  Check for leap year and set the number of days in the month

	gboolean bLeapYear = ((wYear & 3) == 0) && ((wYear % 100) != 0 || (wYear % 400)
			== 0);

	//yuqingzh delete	int nDaysInMonth =_afxMonthDays[wMonth] - _afxMonthDays[wMonth-1] +((bLeapYear && wDay == 29 && wMonth == 2) ? 1 : 0);

	long nDate = wYear * 365L + wYear / 4 - wYear / 100 + wYear / 400
			+ _afxMonthDays[wMonth - 1] + wDay;

	if (wMonth <= 2 && bLeapYear)
		--nDate;

	nDate -= 693959L;

	double dblTime = (((long) wHour * 3600L) + ((long) wMinute * 60L)
			+ ((long) wSecond)) / 86400.;

	double dbTime = (double) nDate + ((nDate >= 0) ? dblTime : -dblTime);
	return dbTime;
}

static void sprintf_mac(char mac_addr[PROLEN_COMPUTERMAC],const guchar mac[6])
{
	sprintf(mac_addr,"%02x%02x%02x%02x%02x%02x",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
}

void RecordAccout(const char * type,in_addr_t ip,in_addr_t destip, const guchar mac[6], const char * host , const char * passwd,const void * data, unsigned short dport,Kpolice * police)
{

	Client * client;

	client = clientmgr_get_client_by_mac(mac);

	if(client && !G_IS_OBJECT(client))
		raise(SIGSEGV);

	client = g_object_ref(client);

	AccountInfo ac;

	GTimeVal time;

	memset(&ac,0,sizeof(ac));

	g_get_current_time(&time);

	gchar * strtime = g_time_val_to_iso8601(&time);

	ac.DateTime = GetDBTime_tm(GetCurrentTime());

	strcpy(ac.SiteID, strHotelID);
	strcpy(ac.SiteName, strHotelName);

	if (client)
	{
		strncpy(ac.CertType, client->idtype,sizeof(ac.CertType) - 1);
		strncpy(ac.CertNo, client->id , sizeof(ac.CertNo) - 1);

		utf8_gbk(ac.ClientName, PROLEN_CLIENTNAME, client->name,strlen(
				client->name));
		if(client->room)
			strncpy(ac.ComputerName,client->room,sizeof(ac.ComputerName));
	}

	snprintf(ac.ComputerIp, sizeof(ac.ComputerIp)-1, "%03d.%03d.%03d.%03d",
			((u_char*) &(ip))[0], ((u_char*) &(ip))[1],
			((u_char*) &(ip))[2], ((u_char*) &(ip))[3]);

	sprintf_mac(ac.ComputerMac,mac);

	strcpy(ac.ServType, type);

	strncpy(ac.Key1, data, 60);
	strncpy(ac.Key2, passwd, sizeof(ac.Key2));

	snprintf(ac.Port, sizeof(ac.Port), "%d", dport);

	snprintf(ac.DestIp, sizeof(ac.DestIp), "%03d.%03d.%03d.%03d",
			((u_char*) &(destip))[0], ((u_char*) &(destip))[1],
			((u_char*) &(destip))[2], ((u_char*) &(destip))[3]);

	kpolice_send_command(police,COMMAND_ACCOUNT, (char *) &ac, sizeof(ac));

	char strmac[32];

	struct in_addr in_addr_ip={0};

	in_addr_ip.s_addr = ip;

	formatMAC((u_char*)mac,strmac);

	static const char	SQL_template[]=
		"insert into t_netlog (RoomNum,MachineIP,MachineMac,CustomerIDType,CustomerIDNum, "
			"CustomerName,nLogType,strLogInfo,nTime) values   ('%s','%s','%s','%s','%s','%s','%s','%s','%s')";


	if (client)
	{
		gchar *ipstr;
		g_object_get(client,"ipstr",&ipstr,NULL);
		ksql_vquery_async(SQL_template, client->room,ipstr, strmac,client->idtype,
				client->id,client->name,type, data, strtime);
		g_free(ipstr);

	}
	else
	{
		ksql_vquery_async(
				"insert into t_netlog (MachineIP,MachineMac,nLogType,strLogInfo,nTime) values   ('%s','%s','%s','%s','%s')",
				inet_ntoa(in_addr_ip), strmac, type, data, strtime);
	}

	g_free(strtime);

	if(client)
		g_object_unref(client);
}

int utf8_gbk(char *outbuf, size_t outlen, const char *inbuf, size_t inlen)
{
	GIConv cd;
	char **pin = (char**)&inbuf;
	char **pout = &outbuf;

	cd =g_iconv_open( "GBK","UTF-8");
	if (cd == 0)
		return -1;

	memset(outbuf, '\0', outlen);
	if (g_iconv(cd, pin, &inlen, pout, &outlen) == (size_t) -1)
	{
		return -1;
	}
	g_iconv_close(cd);

	return 0;
}

int gbk_utf8(char *outbuf, size_t outlen, const char *inbuf, size_t inlen)
{
	GIConv cd;
	char **pin = (char**)&inbuf;
	char **pout = &outbuf;

	cd = g_iconv_open("UTF-8", "GBK");
	if (cd == 0)
		return -1;

	memset(outbuf, '\0', outlen);
	if (g_iconv(cd, pin, &inlen, pout, &outlen) == (size_t) -1)
	{
		return -1;
	}
	g_iconv_close(cd);

	return 0;
}

gboolean verify_id(char * idnum)
{
	static const char W[] =
	{ 7, 9, 10, 5, 8, 4, 2, 1, 6, 3, 7, 9, 10, 5, 8, 4, 2, 1 };
	static const char ex[] =
	{ '1', '0', 'X', '9', '8', '7', '6', '5', '4', '3', '2' };

	int i;
	unsigned long long s ;
	int idlen ;

	idlen = strlen(idnum);

	g_return_val_if_fail(idlen==18||idlen==15,FALSE);

	if(idlen == 15)
		return TRUE;

	for(s = 0,i=0;i<17;i++)
	{
		s +=(idnum[i]-'0' )*W[i];
	}

	return ex[s % 11] == (idnum[17] & 0x5F);
}

gboolean arp_ip2mac(in_addr_t ip,guchar mac[6],int sock)
{

#ifdef DEBUG
	memset(mac,0xe2,6);
	return TRUE;
#endif

	//向内核发送发起ARP查询
	struct arpreq arpr ;
	int ret;
	gboolean success=FALSE;

	memset(&arpr,0,sizeof(struct arpreq));

	arpr.arp_flags = ATF_MAGIC;
	arpr.arp_pa.sa_family = AF_INET;

	memcpy(arpr.arp_pa.sa_data+2,&ip,4);

	struct ifaddrs * ifap , * ifp;

	//获得所有的网络接口
	getifaddrs(&ifap);

	ifp = ifap;

	while( ifp )
	{
		strcpy(arpr.arp_dev,ifp->ifa_name);

		//通过ioctl获得arp
		ret = ioctl(sock, SIOCGARP, &arpr);
		if (ret == 0)
		{
			//获得MAC地址;
			memcpy(mac,arpr.arp_ha.sa_data,6);
			success = TRUE;
			break;
		}
		ifp = ifp->ifa_next;
	}
	//释放获得的接口
	if(ifap)
		freeifaddrs(ifap);
	return success;
}

GKeyFile * gkeyfile;
