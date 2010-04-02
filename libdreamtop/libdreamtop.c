/*
 * libmicrocai.c
 *
 * Copyright 2009 microcai <microcai@sina.com>
 *
 * See COPYING for more details about this software's license
 */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <syslog.h>
#include <sys/ioctl.h>
#include <net/if_arp.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <iconv.h>
#include <errno.h>
#include <string.h>
#include <glib.h>

struct so_data
{
    void* module;
};

typedef int (*PROTOCOL_HANDLER)(struct so_data*,u_char *packet);

#include "libmicrocai-macros.h"
#include "functions.h"

u_int16_t checksum(u_int16_t *buffer, int size)
{
    uint32_t cksum = 0;

    while (size > 1)
    {
        cksum += *buffer++;
        size -= sizeof ( unsigned short);
    }

    if (size)
        cksum += *buffer;

    cksum = (cksum >> 16) + (cksum & 0xffff);

    cksum += (cksum >> 16);

    return (uint16_t) (~cksum);
}

void formatMAC(const u_char * MAC_ADDR,char * strmac)
{
	sprintf(strmac, "%02x:%02x:%02x:%02x:%02x:%02x",
			MAC_ADDR[0], MAC_ADDR[1], MAC_ADDR[2], MAC_ADDR[3], MAC_ADDR[4], MAC_ADDR[5]);

}

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

void  convertMAC(char mac[6],const char * strmac)
{
	int i;
	for (i = 0; i < 6; ++i)
	{
		mac[i] = hex2char(& strmac[i * 3] );
	}
}

#ifdef ENABLE_HOTEL

void run_cmd(const CString & strcmd )
{
	//在这里我不得不考虑system失败会导致的资源泄漏。
	syslog(LOG_NOTICE,"run: %s\n",strcmd.c_str());
	system(strcmd);
}

bool GetMac(const char *ip, char MAC_ADDR[],u_char mac_addr[])
{
	//向内核发送发起ARP查询
	int s = socket(PF_INET, SOCK_DGRAM, 0);

	//遍历所有以太网卡

	int n = 0;
	do
	{
		struct arpreq arpr ={{0}};

		arpr.arp_flags = ATF_MAGIC;
		arpr.arp_pa.sa_family = AF_INET;
		((struct sockaddr_in*) (&(arpr.arp_pa)))->sin_addr.s_addr = inet_addr(ip);

		sprintf(arpr.arp_dev, "eth%d", n);
		n++;
		//通过ioctl获得arp
		int ret = ioctl(s, SIOCGARP, &arpr);
		if (ret == 0)
		{
			close(s);
			//获得MAC地址;
			u_char* d = (u_char*) arpr.arp_ha.sa_data;
			if(mac_addr)
				memcpy(mac_addr,d,6);
			if(MAC_ADDR)
				formatMAC(d,MAC_ADDR);
			return true;
		}
		if (errno == ENXIO)
			continue;
		close (s);
		return false;
	} while(errno != ENODEV);
	close(s);
	return false;
}

#endif
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

GKeyFile *  gkeyfile;
