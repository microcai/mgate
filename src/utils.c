/*
 * utils.c
 *
 *  Created on: 2010-4-8
 *      Author: cai
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/socket.h>
#include <net/ethernet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include "i18n.h"
#include "utils.h"
#include "kpolice.h"
#include "ksql.h"
#include "global.h"

guint64	mac2uint64( guchar mac[6])
{
	return (guint64)mac[0] + ((guint64)mac[1]<<8) + ((guint64)mac[2] <<16) +
			((guint64)mac[3]<<24) + ((guint64)mac[4]<<32) + ((guint64)mac[5] << 40);
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

static void sprintf_mac(char mac_addr[PROLEN_COMPUTERMAC],const char mac[6])
{
	sprintf(mac_addr,"%02x%02x%02x%02x%02x%02x",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
}

void RecordAccout(const char * type,in_addr_t ip,in_addr_t destip, const char mac[6], const char * host , const char * passwd,const void * data, unsigned short dport)
{
#ifdef ENABLE_HOTEL
	struct Clients_DATA cd;
	struct Clients_DATA *pcd = &cd;


	if (get_client_data(mac,pcd)!=0) // there is no ..... so, let's just ignore it.
		return;
#endif

	g_debug("snd ?");


	AccountInfo ac ={0};

//	formattime(strTime);

	ac.DateTime = GetDBTime_tm(GetCurrentTime());

	strcpy(ac.SiteID, strHotelID);
	strcpy(ac.SiteName, strHotelName);

#ifdef ENABLE_HOTEL

	if (pcd && strlen(pcd->mac_addr) < 2)
	{
		GetMac(pcd->ip_addr, pcd->mac_addr, pcd->MAC_ADDR);
	}

	strncpy(ac.CertType, pcd->CustomerIDType,sizeof(ac.CertType)-1);
	strncpy(ac.CertNo, pcd->CustomerID,sizeof(ac.CertNo)-1);

	utf8_gbk(ac.ClientName ,PROLEN_CLIENTNAME, pcd->CustomerName,strlen(pcd->CustomerName));

	snprintf(ac.ComputerName,sizeof(ac.ComputerName),"%c%c%02d",
			*pcd->Build,*pcd->Floor, atoi(pcd->RoomNum));

	snprintf(ac.ComputerIp, sizeof(ac.ComputerIp)-1, "%03d.%03d.%03d.%03d",
			((u_char*) &(pcd->ip))[0], ((u_char*) &(pcd->ip))[1],
			((u_char*) &(pcd->ip))[2], ((u_char*) &(pcd->ip))[3]);
#endif

	sprintf_mac(ac.ComputerMac,mac);

	strcpy(ac.ServType, type);

	snprintf(ac.ComputerIp, sizeof(ac.ComputerIp), "%03d.%03d.%03d.%03d",
			((u_char*) &(ip))[0], ((u_char*) &(ip))[1],
			((u_char*) &(ip))[2], ((u_char*) &(ip))[3]);

	strncpy(ac.Key1, data, 60);
	strncpy(ac.Key2, passwd, sizeof(ac.Key2));

	snprintf(ac.Port, sizeof(ac.Port), "%d", dport);

	snprintf(ac.DestIp, sizeof(ac.DestIp), "%03d.%03d.%03d.%03d",
			((u_char*) &(destip))[0], ((u_char*) &(destip))[1],
			((u_char*) &(destip))[2], ((u_char*) &(destip))[3]);

	kpolice_send_command(COMMAND_ACCOUNT, (char *) &ac, sizeof(ac));

#ifndef ENABLE_HOTEL
//
//	g_string_printf(strSQL,SQL_template, pcd->Build,pcd->Floor,atoi(pcd->RoomNum),
//			pcd->ip_addr,			pcd->mac_addr, pcd->CustomerIDType,
//			pcd->CustomerID, pcd->CustomerName, na->strType,
//			na->data.c_str(), strTime);
#else
	char strmac[32];
	in_addr in_addr_ip={0};

	in_addr_ip.s_addr = na->ip;

	formatMAC(mac,strmac);

	g_string_printf(strSQL,
			"insert into t_netlog (MachineIP,MachineMac,nLogType,strLogInfo,nTime) values   ('%s','%s','%s','%s','%s')",
			inet_ntoa(in_addr_ip),
			strmac, na->strType,
			na->data.c_str(), strTime->str);

#endif
//	ksql_run_query_async(strSQL->str);
//

//

//
//	g_string_free(strTime,1);
//	g_string_free(strSQL,1);
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

