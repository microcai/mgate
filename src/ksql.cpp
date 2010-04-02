/*
 * ksql.cpp
 *
 *  Created on: 2009-10-21
 *      Author: cai
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#ifdef DEBUG
#include <iostream>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <poll.h>
#include <net/ethernet.h>
#include <glib.h>
#include "libdreamtop.h"
#include "./KSQL/kmysql.h"
#include "./KSQL/ksqlite.h"

struct AccountInfo
{
     char		SiteID[PROLEN_NETBARID];
     double		ClientID;
     char		SiteName[PROLEN_NETBARNAME];
     char		ComputerName[PROLEN_COMPUTERNAME];		//机器网络标识
     char		ComputerIp[PROLEN_COMPUTERIP];
     char		ComputerMac[PROLEN_COMPUTERMAC];		//Mac地址
     char		DestIp[PROLEN_COMPUTERIP];
     char		Port[6];
     char		ServType[PROLEN_SERVTYPE];
     char		Key1[PROLEN_KEY1];
     char		Key2[PROLEN_KEY2];
     char		Key3;
     double		DateTime;
     char		ClientName[PROLEN_CLIENTNAME];			// 顾客姓名
     char		CertType[PROLEN_CERTTYPE];
     char    	CertNo[PROLEN_CERTNO];
     char		Organ[PROLEN_ORGAN];
     char		Country[PROLEN_NATIONALITY];
     char		PassWord[PROLEN_PASSWD];
};

static void NOP_SENDDATA(int cmd,void *data,int ulen){syslog(LOG_WARNING,"SendData function not set!\n");}
static const char	SQL_template[]=
	"insert into t_netlog (RoomNum,MachineIP,MachineMac,CustomerIDType,CustomerIDNum, "
		"CustomerName,nLogType,strLogInfo,nTime) values   ('%s%s%02d','%s','%s','%s','%s','%s','%s','%s','%s')";


static FUNC_SENDDATA SendData = NOP_SENDDATA;
int kregisterSendDataFunc(FUNC_SENDDATA f){	SendData = f;return 0;}

void formattime(GString * strtime, struct tm* pTm)
{
	g_string_printf(strtime,"%d-%d-%d %d:%d:%d",
            pTm->tm_year+1900,pTm->tm_mon+1,pTm->tm_mday,
            pTm->tm_hour,pTm->tm_min,pTm->tm_sec);
}

void formattime(GString * strtime)
{
	struct tm pTm;
	time_t t = time(0);
	pTm = *localtime(&t);
	formattime(strtime,&pTm);
}

static void MAC_ADDR2macaddr(char mac_addr[PROLEN_COMPUTERMAC],const u_char mac[ETHER_ADDR_LEN])
{
	sprintf(mac_addr,"%02x%02x%02x%02x%02x%02x",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
}

void RecordAccout(struct CustomerData & cd)
{
	SendData(COMMAND_CUSTOMER, (char *) &cd, sizeof(CustomerData));
}

void RecordAccout(struct NetAcount*na)
{
	u_char * mac = (u_char*)( na->packet + 6);

#ifdef ENABLE_HOTEL
	struct Clients_DATA cd;
	struct Clients_DATA *pcd = &cd;


	if (get_client_data(mac,pcd)!=0) // there is no ..... so, let's just ignore it.
		return;
#endif


	GString * strTime = g_string_new("");
	GString * strSQL = g_string_new("");

	AccountInfo ac ={{0}};

	formattime(strTime);

	ac.DateTime = GetDBTime_tm(GetCurrentTime());

	strcpy(ac.SiteID, hotel::strHotelID);
	strcpy(ac.SiteName, hotel::strHoteName);

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

	MAC_ADDR2macaddr(ac.ComputerMac,mac);

	strcpy(ac.ServType, na->strType);

#ifdef ENABLE_HOTEL

	g_string_printf(strSQL,SQL_template, pcd->Build,pcd->Floor,atoi(pcd->RoomNum),
			pcd->ip_addr,			pcd->mac_addr, pcd->CustomerIDType,
			pcd->CustomerID, pcd->CustomerName, na->strType,
			na->data.c_str(), strTime);
#else
	char strmac[32];
	in_addr in_addr_ip={0};

	in_addr_ip.s_addr = na->ip;

	snprintf(ac.ComputerIp, sizeof(ac.ComputerIp), "%03d.%03d.%03d.%03d",
			((u_char*) &(in_addr_ip))[0], ((u_char*) &(in_addr_ip))[1],
			((u_char*) &(in_addr_ip))[2], ((u_char*) &(in_addr_ip))[3]);

	formatMAC(mac,strmac);

	g_string_printf(strSQL,
			"insert into t_netlog (MachineIP,MachineMac,nLogType,strLogInfo,nTime) values   ('%s','%s','%s','%s','%s')",
			inet_ntoa(in_addr_ip),
			strmac, na->strType,
			na->data.c_str(), strTime->str);

#endif
	ksql_run_query_async(strSQL->str);
//	syslog(LOG_NOTICE,"%s",strSQL.c_str());

	strncpy(ac.Key1, na->data.c_str(), 60);
	strncpy(ac.Key2, na->passwd.c_str(), sizeof(ac.Key2));

	snprintf(ac.DestIp, sizeof(ac.DestIp), "%03d.%03d.%03d.%03d",
			((u_char*) &(na->dstip))[0], ((u_char*) &(na->dstip))[1],
			((u_char*) &(na->dstip))[2], ((u_char*) &(na->dstip))[3]);

	snprintf(ac.Port, sizeof(ac.Port), "%d", na->dport);

#ifdef DEBUG

	std::cout << "ID: " << ac.CertNo << std::endl;
	std::cout << "IDTYPE: " << ac.CertType << std::endl;
	std::cout << "帐号:" << ac.Key1 << std::endl;
	std::cout << "密码:" << ac.Key2 << std::endl;
	std::cout << "主机:" << ac.DestIp;
#ifdef ENABLE_HOTEL
	std::cout << "用户名:" << pcd->CustomerName << std::endl;
#endif
	std::cout << "机器IP:" << ac.ComputerIp << std::endl;
#endif
	SendData(COMMAND_ACCOUNT, (char *) &ac, sizeof(ac));
	g_string_free(strTime,1);
	g_string_free(strSQL,1);
}



static volatile int	ksql_inited=false;
static volatile int	ksql_usemysql=false;

void ksql_query_and_use_result(void(*callback)(KSQL_ROW row, void*p),
		const char* query, void*p)
{
	KSQL_RES * res;
	KSQL_ROW	row;
	res = ksql_query_and_use_result(query); //内部都开始使用自己封装的函数了。呵呵
	while ((row = ksql_fetch_row(res)))
	{
		if (callback) //避免错误啊，呵呵
			callback(row, p);
	}
	ksql_free_result(res);
}

void InsertCustomerLog(const char * build,const char * floor,const char * room, const char * name ,
		const char * idtype , const char * id, const char * type,const char * ip, const char * mac, const char * time)
{
	GString *  sqlstr = g_string_new("");

	g_string_printf(sqlstr,"insert into t_customerlog ("
			"BuildNum,RoomFloor,RoomNum,CustomerName,CustomerIDType,"
			"CustomerIDNum,MachineIP,MachineMac,nType,HappyTime) "
			"values ('%s','%s','%s','%s','%s','%s','%s','%s','%s','%s')",
			build,floor,room,name,idtype,id,ip,mac,type,time);
	//log_printf(L_DEBUG_OUTPUT,"%s\n",sqlstr.c_str());
	ksql_run_query_async(sqlstr->str);
	g_string_free(sqlstr,TRUE);
}

KSQL_ROW ksql_fetch_row(KSQL_RES*res)
{
	if (res)
	{
		if (ksql_usemysql)
			return kmysql_fetch_row(res);
		else
			return ksqlite_fetch_row(res);
	}
	return NULL;
}

static int	ksql_daemon_socket=0;
static void * KSQL_daemon(void*_p)
{
	int ksql_daemon_socket_recv ;
	pollfd	pfd;

	char	buf[4096];

	struct
	{
		pthread_mutex_t p;
		int fd[2];
	} *p = (typeof(p))_p;

	ksql_daemon_socket_recv = p->fd[0];
	shutdown(ksql_daemon_socket_recv,1);

	pthread_mutex_unlock(&p->p);

	KSQL_RES * res;
	KSQL_ROW	row;

	const gchar * pswd, *user, *host, *database;

	extern GKeyFile * gkeyfile;

	pswd = g_key_file_get_string(gkeyfile,"monitor","db.config.password",NULL);
	if(!pswd) pswd = "";
	user = g_key_file_get_string(gkeyfile,"monitor","db.config.username",NULL);
	if(!user) user = "root";

	host = g_key_file_get_string(gkeyfile,"monitor","db.config.host", NULL);
	if(!host) host = "localhost";

	database = g_key_file_get_string(gkeyfile,"monitor","db.config.dbname", NULL);
	if(!host)
		host = "hotel";

#ifdef WITH_SQLITE3
	//初始化 SQLite 数据库。不可以失败!
	InitSqlite();
#endif

	for (;;)
	{
		ksql_inited = 0;

#ifdef WITH_SQLITE3
		if (InitMySQL(pswd, user, database, host))
		{
			syslog(LOG_ERR, "Cannot connect to MYSQL server, use SQLite !");
		}else
			ksql_usemysql = true;

#else
		while (InitMySQL(pswd, user, database, host))
		{
			syslog(LOG_ERR, "Cannot connect to MYSQL server, sleep and retry!");
			sleep(2);
		}
		ksql_usemysql = true;
#endif

		//Get t_sysparam
		res = (typeof(res)) ksql_query_and_use_result(
				"select * from t_sysparam");

		row = ksql_fetch_row(res);
		if (row)
		{
			strcpy(hotel::strServerIP, row[1]);
			strcpy(hotel::strHotelID, row[2]);
			strcpy(hotel::str_ethID + 3, row[3]);

			utf8_gbk(hotel::strHoteName, sizeof(hotel::strHoteName), row[4],
					strlen(row[4]));

#ifdef ENABLE_HOTEL
			strcpy(hotel::strWebIP, row[5]);
#endif

			syslog(LOG_NOTICE, "hotel name is %s\n", row[4]);
			syslog(LOG_NOTICE, "hotel ID is %s\n", hotel::strHotelID);
			syslog(LOG_NOTICE, "ServerIP is %s\n", hotel::strServerIP);
#ifdef ENABLE_HOTEL
			syslog(LOG_NOTICE,"WebIP is %s\n",hotel::strWebIP);
#endif
		}
		else if (!res)
		{
			syslog(LOG_WARNING, "tables not exist, create them.\n");
			if(ksql_usemysql)
				InitMysqlDB();
			InitSqliteDB();
			syslog(LOG_NEWS, "All tables created!");
			exit(0);
		}
		ksql_free_result(res);
#ifdef ENABLE_HOTEL
		//初始化跳转页面
		GString * dest = g_string_new("");
		g_string_printf(dest,"%s/login", hotel::strWebIP);

		init_http_redirector(dest->str);
		g_string_free(dest,1);
#endif

	//------------------------------------------
		//现在，开始检测是否断开了数据库的连接，呵呵，断开连接就重新连接上
		//并且为异步做服务，哈哈
		ksql_inited = 2;

		for(;;)
		{
			pfd.fd = ksql_daemon_socket_recv;

			pfd.events = POLLIN;

			while(poll(&pfd,1,10000))
			{
				//执行代码
				(void)read(ksql_daemon_socket_recv,buf,4096);
				printf("%s\n",buf);
				if (ksql_run_query(buf))
					break;
			}

			if (is_mysqlserver_gone())
			{
				kmysql_close();
				break;
			}else{
				//修复数据库，神州行，我看行。
				ksql_run_query("repair table t_netlog");
			}
		}
	}
	return 0;
}


void StartSQL()
{
	// start the daemon thread, and let the daemon stuff determing the fucking stuff,haha
	if(ksql_daemon_socket)return ;

	struct
	{
		pthread_mutex_t p;
		int fd[2];
	} p;

	socketpair(AF_LOCAL, SOCK_CLOEXEC | SOCK_SEQPACKET, 0, p.fd);

	ksql_daemon_socket = p.fd[1];
	shutdown(ksql_daemon_socket,0);

	pthread_mutex_init(&p.p, 0);
	pthread_mutex_lock(&p.p);

	pthread_t ksql_daemon;

	pthread_create(&ksql_daemon, 0,
			KSQL_daemon, &p);

	pthread_detach(ksql_daemon);
	pthread_mutex_lock(&p.p);
	pthread_mutex_unlock(&p.p);
	pthread_mutex_destroy(&p.p);

	syslog(LOG_INFO,"starting ksql daemon");
}


int WaitForSQLserver()
{
	while (ksql_inited!=2)
	{
		sched_yield();
	}

	return 0;
}

int ksql_run_query(const char *p)
{
	int ret;
	if(ksql_usemysql)
	{
		ret = kmysql_run_query(p);
	}else
	ret = ksqlite_query(p);
	if (ret)
		syslog(LOG_ERR, "err make query  %s\n", p);
	return ret;
}

int ksql_run_query_async(const char *p)
{
	int ret = write(ksql_daemon_socket, p , strlen(p) + 1);
	return ret<0?1:0;
}

KSQL_RES* ksql_query_and_use_result(const char* query)
{
	if(ksql_usemysql)
	{
		return kmysql_query_and_use_result(query);
	}else
	return ksqlite_query_and_use_result(query);
}

void ksql_free_result(KSQL_RES* res)
{
	if(ksql_usemysql)
	{
		kmysql_free_result(res);
	}else
	{
		ksqlite_free_result(res);
	}

}

namespace hotel{
    char strHotelID[32];
    char strHoteName[32];
    char strServerIP[32];
    char strWebIP[32];
    char str_ethID[32]="eth1";
    bool Is_Old_DB=false;
}
