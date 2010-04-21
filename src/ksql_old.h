/*
 * ksql.h
 *
 *  Created on: 2009-10-21
 *      Author: cai
 */

#ifndef KSQLdd_H_
#define KSQLdd_H_


/*
 * File:   kmysql.h
 * Author: king
 *
 * Created on 2009年4月22日, 下午4:32
 */


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


void formattime(std::string & strtime, struct tm* pTm);
void formattime(std::string & strtime);

typedef void (*FUNC_SENDDATA)(int cmd,void *data,int ulen);



void RecordAccout(struct NetAcount*na);
void RecordAccout(struct CustomerData & cd);
void InsertCustomerLog(const char * build,const char * floor,const char * room, const char * name ,
		const char * idtype , const char * id, const char * type,const char * ip, const char * mac, const char * time);


#endif /* KSQL_H_ */
