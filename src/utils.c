/*
 * utils.c
 *
 *  Created on: 2010-4-8
 *      Author: cai
 */


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


guint64	mac2uint64( guchar mac[6])
{

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

