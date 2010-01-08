

/*
 * 	mysql.cpp
 *
 * 	Copyright 2009 microcai(microcai@sina.com)
 *
 *	microcai's Wrapper for MySQL client lib
 *
 */

#include <iostream>
#include <map>

#include <syscall.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <net/ethernet.h>
#include <pthread.h>
#include <mysql/mysql.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#include "libmicrocai.h"
#define		__KLIBSQL_USEINTERNALLY

#include "kmysql.h"
#include "my_log.h"

void** ksql_query_and_use_result(const char* query);
void** ksql_query_and_use_result_quite(const char* query);

static pthread_mutex_t sql_mutex=PTHREAD_MUTEX_INITIALIZER;

static MYSQL mysql;

static FUNC_SENDDATA SendData;

namespace hotel{
    char strHotelID[32];
    char strHoteName[32];
    char strServerIP[32];
    char strWebIP[32];
    char str_ethID[32];
}

int kregisterSendDataFunc(FUNC_SENDDATA f){	SendData = f;}

double GetDBTime(char *pTime)
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


    int _afxMonthDays[13] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365};

    unsigned short int wYear = nYear;
    unsigned short int wMonth = nMon;
    unsigned short int wDay = nDay;
    unsigned short int wHour = nHour;
    unsigned short int wMinute = nMin;
    unsigned short int wSecond = nSec;

    //  Check for leap year and set the number of days in the month

    bool bLeapYear = ((wYear & 3) == 0) && ((wYear % 100) != 0 || (wYear % 400) == 0);

    //yuqingzh delete	int nDaysInMonth =_afxMonthDays[wMonth] - _afxMonthDays[wMonth-1] +((bLeapYear && wDay == 29 && wMonth == 2) ? 1 : 0);

    long nDate = wYear * 365L + wYear / 4 - wYear / 100 + wYear / 400 + _afxMonthDays[wMonth - 1] + wDay;

    if (wMonth <= 2 && bLeapYear)
        --nDate;

    nDate -= 693959L;

    double dblTime = (((long) wHour * 3600L) + ((long) wMinute * 60L) + ((long) wSecond)) / 86400.;

    double dbTime = (double) nDate + ((nDate >= 0) ? dblTime : -dblTime);
    return dbTime;
}

static void formattime(std::string & strtime, struct tm* pTm)
{
	char *st = new char [50];
//	time_t t = time(0);
//	pTm = localtime(&t);
    sprintf(st,"%d-%d-%d %d:%d:%d",
            pTm->tm_year+1900,pTm->tm_mon+1,pTm->tm_mday,
            pTm->tm_hour,pTm->tm_min,pTm->tm_sec);
    strtime = st;
    delete []st;
}
static void formattime(std::string & strtime)
{
	struct tm pTm;
	time_t t = time(0);
	pTm = *localtime(&t);
	formattime(strtime,&pTm);
}

typedef struct AccountInfo{
    char SiteID[PROLEN_NETBARID];
    double ClientID;
    char SiteName[PROLEN_NETBARNAME];
    char ComputerName[PROLEN_COMPUTERNAME]; //机器网络标识
    char ComputerIp[PROLEN_COMPUTERIP];
    char ComputerMac[PROLEN_COMPUTERMAC]; //Mac地址
    char DestIp[PROLEN_COMPUTERIP];
    char Port[6];
    char ServType[PROLEN_SERVTYPE];
    char Key1[PROLEN_KEY1];
    char Key2[PROLEN_KEY2];
    char Key3;
    double DateTime;
    char ClientName[PROLEN_CLIENTNAME]; // 顾客姓名
    char CertType[PROLEN_CERTTYPE];
    char CertNo[PROLEN_CERTNO];
    char Organ[PROLEN_ORGAN];
    char Country[PROLEN_NATIONALITY];
    char PassWord[PROLEN_PASSWD];
}__attribute__((packed)) AccountInfo;

static const char	SQL_template[]=
	"insert into T_NetLog (RoomNum,MachineIP,MachineMac,CustomerIDType,CustomerIDNum, "
		"CustomerName,nLogType,strLogInfo,nTime) values ('%s','%s','%s','%s','%s','%s','%s','%s','%s')";


void RecordAccout(struct NetAcount*na)
{
	struct Clients_DATA * pcd;

	pcd = get_client_data(na->ip);
	struct in_addr ip_addr;
	std::string strTime;
	char* strSQL;

	AccountInfo ac =
	{ 0 };

	switch (na->type)
	{
	case NetAcountType_MAC_ADDR:
		//上下机的记录
		formattime(strTime, &(pcd->onlinetime));
		ip_addr.s_addr = pcd->ip;
		strcpy(ac.CertType, pcd->CustomerIDType.c_str());
		strcpy(ac.SiteID, hotel::strHotelID);
		strcpy(ac.SiteName, hotel::strHoteName);
		ac.DateTime = GetDBTime((char*) strTime.c_str());
		strcpy(ac.ClientName, pcd->CustomerName.c_str());
		strcpy(ac.CertNo, pcd->CustomerID.c_str());
		strcpy(ac.ComputerIp, inet_ntoa(ip_addr));
		strcpy(ac.ComputerMac, pcd->MAC_ADDR);
		strcpy(ac.ComputerName, pcd->RoomNum.c_str());
		strcpy(ac.ServType, na->strType);

		printf("房间号:%s\n", ac.ComputerName);
		printf("用户名:%s\n", ac.ClientName);
		printf("ID号码:%s\n", ac.CertNo);
		printf("机器IP:%s\n", ac.ComputerIp);
		SendData(COMMAND_CUSTOMER, (char *) &ac, sizeof(ac));
		break;

	case NetAcountType_QQ:
	case NetAcountType_MSN:
	case NetAcountType_HTTP:
	case NetAcountType_POST:
	default:
	{
		ip_addr.s_addr = na->ip;

		formattime(strTime);
		strSQL = new char[2048];

		for (int t=0;t<2;++t) //最多就循环两次，避免死循环
		{
			sprintf(strSQL, SQL_template, pcd->RoomNum.c_str(), pcd->ip_addr.c_str(),
					pcd->mac_addr.c_str(), pcd->CustomerIDType.c_str(), pcd->CustomerID.c_str(),
					pcd->CustomerName.c_str(), na->type, na->data.c_str(),
					strTime.c_str());

			if (ksql_run_query(strSQL))
			{
				char mac[32];
				if (GetMac(pcd->ip_addr.c_str(), mac, pcd->MAC_ADDR))
					pcd->mac_addr = mac;
				else
					break;
			}
			else
				break;
		}
		delete[] strSQL;

		strcpy(ac.SiteID, hotel::strHotelID);
		strcpy(ac.SiteName, hotel::strHoteName);
		strcpy(ac.ServType, na->strType);
		strcpy(ac.ComputerIp, pcd->ip_addr.c_str());
		strcpy(ac.ComputerMac, pcd->MAC_ADDR);
		strcpy(ac.ComputerMac, pcd->RoomNum.c_str());
		strcpy(ac.Key1, na->data.c_str());
		// strcpy(Account.Key2, row[10]);
		//@NOTE: strcpy(ac.Key2,);
		ip_addr.s_addr = na->dstip;
		strcpy(ac.DestIp, inet_ntoa(ip_addr));
		ac.DateTime = GetDBTime((char*) strTime.c_str());
		strcpy(ac.ClientName, pcd->CustomerName.c_str());
		strcpy(ac.CertType, pcd->CustomerIDType.c_str());
		strcpy(ac.CertNo, pcd->CustomerID.c_str());

		std::cout << "场所编号: " << ac.SiteID << std::endl;

		std::cout << "帐号类型: " << ac.ServType << std::endl;
		std::cout << "帐号:" << ac.Key1 << std::endl;
		std::cout << "密码:" << ac.Key2 << std::endl;
		std::cout << "主机:" << ac.DestIp;
		std::cout << "用户名:" << ac.ClientName << std::endl;
		std::cout << "ID号码:" << ac.CertNo << std::endl;
		std::cout << "机器IP:" << ac.ComputerIp << std::endl;

		SendData(COMMAND_ACCOUNT, (char *) &ac, sizeof(ac));
		break;
	}
	}
}

void RecordNetAccount(std::string & pType,std::string & Log,in_addr_t ip)
{
    std::string strTime;
    char * strSQL = new char[2048];
    struct Clients_DATA *pcd;
    in_addr in_addr_ip;
    in_addr_ip.s_addr = ip;

    pcd = get_client_data(ip);

    formattime(strTime);

    sprintf(strSQL,
                "insert into T_NetLog (RoomNum,MachineIP,MachineMac,CustomerIDType,CustomerIDNum, "
                "CustomerName,nLogType,strLogInfo,nTime) values ('%s','%s','%s','%d','%s','%s','%s','%s','%s')",
                pcd->RoomNum.c_str(),inet_ntoa(in_addr_ip),pcd->MAC_ADDR,pcd->CustomerIDType.c_str(),
                pcd->CustomerID.c_str(),pcd->CustomerName.c_str(),  pType.c_str(),Log.c_str(),strTime.c_str()
            );
    ksql_run_query(strSQL);
    delete[] strSQL;
}


int InitRecordSQL(const std::string & passwd, const std::string & user,
		const std::string & database, const std::string & host)
{
	MYSQL_RES *res;
    MYSQL_ROW row;

    mysql_thread_init();

 	mysql_init(&mysql);
 	pthread_mutex_init(&sql_mutex,0);

 	pthread_mutex_lock(&sql_mutex);
    if(!mysql_real_connect(&mysql,host.c_str(),user.c_str(),passwd.c_str(),database.c_str(), 0, 0, 0))
    {
    	pthread_mutex_unlock(&sql_mutex);
        std::cerr << "Error connecting to database:"
                << mysql_error(&mysql) << std::endl;
        return mysql_errno(&mysql);
    }
    pthread_mutex_unlock(&sql_mutex);

    //Get t_sysparam
	res = (MYSQL_RES*) ksql_query_and_use_result("select * from t_sysparam");
	row = ksql_fetch_row(res);
	if (row)
	{

		strcpy(hotel::strServerIP, row[0]);
		strcpy(hotel::strWebIP, row[4]);
		strcpy(hotel::strHotelID, row[1]);
		strcpy(hotel::strHoteName, row[2]);
		strcpy(hotel::str_ethID, row[3]);

		std::cout << "ServerIP is " << hotel::strServerIP << std::endl;
		std::cout << "WebIP is " << hotel::strWebIP << std::endl;

	}
	ksql_free_result(res);
    return 0;
}

int ksql_run_query(const char *p)
{
	pthread_mutex_lock(&sql_mutex);
	int ret = mysql_query(&mysql, p);
	pthread_mutex_unlock(&sql_mutex);
	//尽量减少加锁的时间，增加并行。
	if (ret)
		log_printf(L_DEBUG_OUTPUT,"err make query  %s\n",mysql_error(&mysql));
	return ret;
}
MYSQL_ROW ksql_fetch_row(MYSQL_RES*res)
{
	if(res)
		return mysql_fetch_row(res);
	else
		return NULL;
}

void ksql_free_result(MYSQL_RES* res)
{
	if (res)
	{
		mysql_free_result((MYSQL_RES*) res);
//		std::cout << mysql_error(&mysql) << std::endl;
//		pthread_mutex_unlock(&sql_mutex);
	}
}
void* * ksql_query_and_use_result(const char* query)
{
	MYSQL_RES *	res;

	pthread_mutex_lock(&sql_mutex);

	if(mysql_query(&mysql,query))
	{
		pthread_mutex_unlock(&sql_mutex);
		log_printf(L_DEBUG_OUTPUT,"err make query  %s\n",mysql_error(&mysql));
		return NULL;
	}
	res = mysql_store_result(&mysql);

	pthread_mutex_unlock(&sql_mutex);

	return (void**)res;
}
void ksql_query_and_use_result( void (*callback)( MYSQL_ROW row,void*p ),const char* query,void*p)
{
	MYSQL_RES* res;
	MYSQL_ROW	row;
	res = (MYSQL_RES*)ksql_query_and_use_result(query); //内部都开始使用自己封装的函数了。呵呵
	while ((row = ksql_fetch_row(res)))
	{
		if(callback) //避免错误啊，呵呵
			callback(row, p);
	}
	ksql_free_result(res);
}

void** ksql_query_and_use_result_quite(const char* query)
{
	MYSQL_RES *	res;
	pthread_mutex_lock(&sql_mutex);

	if(mysql_query(&mysql,query))
	{
		pthread_mutex_unlock(&sql_mutex);
		log_printf(L_DEBUG_OUTPUT,"err make query  %s\n",mysql_error(&mysql));
		return NULL;
	}
	res = mysql_store_result(&mysql);
	pthread_mutex_unlock(&sql_mutex);
	return (void**) res;
}
void ksql_thread_init()
{
	mysql_thread_init();
}
void ksql_thread_end()
{
	mysql_thread_end();
}
