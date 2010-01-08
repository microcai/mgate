/*
 * File:   kmysql.h
 * Author: king
 *
 * Created on 2009年4月22日, 下午4:32
 */

#ifndef _KMYSQL_H
#define	_KMYSQL_H
#ifndef __cplusplus
#error ("需要C++编译器支持")
#endif
#pragma once

#include <netinet/in.h>

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
#define PROLEN_KEY1		61
#define PROLEN_ORGAN		71
#define PROLEN_KEY2		81
#define PROLEN_SENDTITLE	101
#define PROLEN_REMARK		1024
#define PROLEN_MSGCONTENT	1024

#define COMMAND_STATUS 13000
#define COMMAND_CUSTOMER 13001
#define COMMAND_ACCOUNT 13004
#define COMMAND_ROOM_LIST 13005
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
}

#ifndef _mysql_h
typedef char** MYSQL_ROW;
typedef void * MYSQL_RES;
#endif
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
    NetAcount(enum NetAcountType type,u_char * _packet)
    {
    	ip = dstip = 0;
    	memset(strType,0,8);
    	host  = "0" ;
    	passwd = "0";
    	data = "0";
    	packet = (char*) _packet;
    }
};


void RecordAccout(struct NetAcount*na);
extern "C"
{

int InitRecordSQL(const std::string & passwd = std::string(""),
		const std::string & user = std::string("root"),
		const std::string & database = std::string("hotel"),
		const std::string & host = std::string("localhost"));

void ksql_query_and_use_result(void(*callback)(MYSQL_ROW row, void*),
		const char* query, void*p = NULL);
int ksql_run_query(const char *p);

#ifndef _mysql_h
// 使用  libksql 的定义

	/*
	 * 成功 返回 MYSQL_RES* 类型，可以用于随后的 ksql_fetch_row 和 ksql_free_result
	 * 失败返回 NULL。不用检查此值，直接传给 ksql_fetch_row ，用
	 * while((row = ksql_fetch_row(res))
	 * {
	 * 		// DO some thing
	 * }
	 * ksql_free_result(res)
	 * 这样的方式就可以了。
	 */
	extern "C++" MYSQL_RES* ksql_query_and_use_result(const char* query);


	/*
	 * 同 ksql_query_and_use_result ，不同的地方在于失败不会在标准错误输出上打印失败原因，
	 * 但是会在内存记录 log 里添加失败时间和原因。
	 */
	extern "C++" MYSQL_RES* ksql_query_and_use_result_quite(const char* query);
#else
#ifndef __KLIBSQL_USEINTERNALLY
#error ("必须不可以包含 <mysql/mysql.h>")
#endif
#endif

MYSQL_ROW ksql_fetch_row(MYSQL_RES*res);
void ksql_free_result(MYSQL_RES* res);
void ksql_thread_init();
void ksql_thread_end();
}

double GetDBTime(char *pTime);

typedef void (*FUNC_SENDDATA)(int cmd,void *data,int ulen);

int kregisterSendDataFunc(FUNC_SENDDATA);


#endif	/* _KMYSQL_H */

