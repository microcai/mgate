/*
 * ksql.h
 *
 *  Created on: 2009-10-21
 *      Author: cai
 */

#ifndef KSQL_H_
#define KSQL_H_


/*
 * File:   kmysql.h
 * Author: king
 *
 * Created on 2009年4月22日, 下午4:32
 */

#ifndef __cplusplus
#error ("需要C++编译器支持")
#endif
#pragma once

#include <netinet/in.h>
#include <string>

#define PROLEN_NATIONALITY	4
#define PROLEN_CERTTYPE		4
#define PROLEN_COMPUTERCODE	4
#define PROLEN_COMPUTERNAME	5
#define PROLEN_SERVTYPE		5
#define PROLEN_TAG		5
#define PROLEN_AUDITAREA	7
#define PROLEN_NETBARSERVERVER	11
#define PROLEN_COMPUTERMAC	13
#define PROLEN_NETBARID		15
#define PROLEN_MSGID		16
#define PROLEN_CERTNO		19
#define PROLEN_NETBARINSIDEIN	21
#define PROLEN_CARDNO		21
#define PROLEN_GACLIENTID	21
#define PROLEN_GACLIENTUSERNAME	31
#define PROLEN_CLIENTNAME	31
#define PROLEN_UNIT		31
#define PROLEN_COMPUTERIP	40
#define PROLEN_NETBARNAME	41
#define PROLEN_PASSWD		51

#ifdef ENABLE_BIGKEY1
#define PROLEN_KEY1		501
#else
#define PROLEN_KEY1		61
#endif

#define PROLEN_ORGAN		71
#define PROLEN_KEY2		81
#define PROLEN_SENDTITLE	101
#define PROLEN_REMARK		1024
#define PROLEN_MSGCONTENT	1024

#define COMMAND_STATUS 13000
#define COMMAND_CUSTOMER 13001
#define COMMAND_ACCOUNT 13004
#define COMMAND_ROOM_LIST 13005
#define COMMAND_HARTBEAT 13111
#define COMMAND_ROOM_LIST_T 45000
#define STATUS_ONLINE 1
#define STATUS_LOGIN 0
#define STATUS_QUIT 2


namespace hotel{
    extern char strHotelID[32];
    extern char strHoteName[32];
    extern char strServerIP[32];
    extern char strWebIP[32];
    extern char str_ethID[32];
    extern bool	Is_Old_DB;
}

enum NetAcountType{
    NetAcountType_MAC_ADDR=1000,
    NetAcountType_MSN = 1001,
    NetAcountType_QQ = 1002,
    NetAcountType_HTTP = 1003,
    NetAcountType_POST = 1004,
    NetAcountType_LZ   = 1005
};

struct NetAcount{
    enum NetAcountType type;
    char strType[8];
    in_addr_t   ip;
    in_addr_t   dstip;
    char*	packet;
    std::string    host;
    std::string passwd;
    std::string data;
    u_short dport;
    NetAcount(enum NetAcountType type,u_char * _packet)
    {
    	ip = dstip = 0;
    	memset(strType,0,8);
    	host  = "0" ;
    	passwd = "";
    	data = "0";
    	packet = (char*) _packet;
    }
};

void ksql_query_and_use_result(void(*callback)(KSQL_ROW row, void*),
		const char* query, void*p = NULL);
int  ksql_run_query(const char *p);
int	 ksql_run_query_async(const char *p);

void StartSQL();
int WaitForSQLserver();

KSQL_RES* ksql_query_and_use_result(const char* query);


KSQL_ROW ksql_fetch_row(KSQL_RES*res);
void ksql_free_result(KSQL_RES* res);
void ksql_thread_init();
void ksql_thread_end();

void formattime(std::string & strtime, struct tm* pTm);
void formattime(std::string & strtime);
double GetDBTime(char *pTime);
double GetDBTime(struct tm *);

typedef void (*FUNC_SENDDATA)(int cmd,void *data,int ulen);

int kregisterSendDataFunc(FUNC_SENDDATA);

struct CustomerData {
	char SiteID[15];
	char AuditArea[9];
	double ClientID;
	char SiteName[48];
	double DateTime;
	char ClientName[PROLEN_CLIENTNAME];
	char CertType[PROLEN_CERTTYPE];
	char CertNo[PROLEN_CERTNO];
	char ComputerName[PROLEN_COMPUTERNAME];
	char ComputerIp[PROLEN_COMPUTERIP];
	char ComputerMac[PROLEN_COMPUTERMAC];
	char Unit[PROLEN_UNIT]; //�û�סַ
	char Organ[PROLEN_ORGAN];//��֤���
	char Country[PROLEN_NATIONALITY];
	u_char Type;
	char Tags[PROLEN_TAG];
	char CardNo[PROLEN_CARDNO];
	char Remark[PROLEN_REMARK];
	CustomerData()
	{
		memset(this,0,sizeof(CustomerData));
		memcpy(AuditArea,SiteID,6);
		strncpy(SiteID,hotel::strHotelID,PROLEN_NETBARID);
		strncpy(SiteName,hotel::strHoteName,PROLEN_NETBARNAME);
	}
};//__attribute__((packed));

void RecordAccout(struct NetAcount*na);
void RecordAccout(struct CustomerData & cd);
void InsertCustomerLog(const char * build,const char * floor,const char * room, const char * name ,
		const char * idtype , const char * id, const char * type,const char * ip, const char * mac, const char * time);


#endif /* KSQL_H_ */
