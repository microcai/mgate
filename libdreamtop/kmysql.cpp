

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
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <fcntl.h>
#include <syscall.h>
#include <unistd.h>
#include <pthread.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <net/ethernet.h>
#include <mysql/mysql.h>
#include <mysql/errmsg.h>

#define		__KLIBSQL_USEINTERNALLY
#include "libmicrocai.h"

#define __PACKED__

void** ksql_query_and_use_result(const char* query);

static pthread_mutex_t sql_mutex;

static MYSQL mysql;

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

static void NOP_SENDDATA(int cmd,void *data,int ulen)
{
	log_printf(L_WARNING,"SendData function not set!\n");
	return ;
}

static FUNC_SENDDATA SendData = NOP_SENDDATA;

namespace hotel{
    char strHotelID[32];
    char strHoteName[32];
    char strServerIP[32];
    char strWebIP[32];
    char str_ethID[32]="eth0";
    bool Is_Old_DB=false;
}

int kregisterSendDataFunc(FUNC_SENDDATA f){	SendData = f;return 0;}

void formattime(std::string & strtime, struct tm* pTm)
{
	CString st;
    st.Format("%d-%d-%d %d:%d:%d",
            pTm->tm_year+1900,pTm->tm_mon+1,pTm->tm_mday,
            pTm->tm_hour,pTm->tm_min,pTm->tm_sec);
    strtime = st.c_str();
}

void formattime(std::string & strtime)
{
	struct tm pTm;
	time_t t = time(0);
	pTm = *localtime(&t);
	formattime(strtime,&pTm);
}

static const char	SQL_template[]=
	"insert into t_netlog (RoomNum,MachineIP,MachineMac,CustomerIDType,CustomerIDNum, "
		"CustomerName,nLogType,strLogInfo,nTime) values   ('%s%s%02d','%s','%s','%s','%s','%s','%s','%s','%s')";


static void MAC_ADDR2macaddr(char mac_addr[PROLEN_COMPUTERMAC],const u_char mac[ETHER_ADDR_LEN])
{
	sprintf(mac_addr,"%02x%02x%02x%02x%02x%02x",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
}

void RecordAccout(struct NetAcount*na)
{
	struct Clients_DATA cd;
	struct Clients_DATA *pcd = &cd;

	;

	if (get_client_data(na->ip,pcd)!=0) // there is no ..... so, let's just ignore it.
		return;

	std::string strTime;
	CString strSQL;

	AccountInfo ac ={{0}};

	formattime(strTime);

	ac.DateTime = GetDBTime(GetCurrentTime());


	if (pcd && pcd->mac_addr.size() < 2)
	{
		char mac[32];
		if (GetMac(pcd->ip_addr.c_str(), mac, pcd->MAC_ADDR))
			pcd->mac_addr = mac;
	}

	strcpy(ac.SiteID, hotel::strHotelID);
	strcpy(ac.SiteName, hotel::strHoteName);

	strcpy(ac.CertType, pcd->CustomerIDType.c_str());
	strcpy(ac.CertNo, pcd->CustomerID.c_str());

	utf8_gbk(ac.ClientName ,PROLEN_CLIENTNAME, pcd->CustomerName.c_str(),pcd->CustomerName.length());

	snprintf(ac.ComputerIp, sizeof(ac.ComputerIp), "%03d.%03d.%03d.%03d",
			((u_char*) &(pcd->ip))[0], ((u_char*) &(pcd->ip))[1],
			((u_char*) &(pcd->ip))[2], ((u_char*) &(pcd->ip))[3]);

	MAC_ADDR2macaddr(ac.ComputerMac, pcd->MAC_ADDR);
	snprintf(ac.ComputerName,sizeof(ac.ComputerName),"%s%s%02d",
			pcd->Build.c_str(),pcd->Floor.c_str(), atoi(pcd->RoomNum.c_str()));

	strcpy(ac.ServType, na->strType);

	strSQL.Format(SQL_template, pcd->Build.c_str(),pcd->Floor.c_str(),atoi(pcd->RoomNum.c_str()), pcd->ip_addr.c_str(),
			pcd->mac_addr.c_str(), pcd->CustomerIDType.c_str(),
			pcd->CustomerID.c_str(), pcd->CustomerName.c_str(), na->strType,
			na->data.c_str(), strTime.c_str());
	ksql_run_query(strSQL);

#ifdef DEBUG
		log_printf(L_DEBUG_OUTPUT, "%s\n",strSQL.c_str());
#endif
//	formattime(strTime,pcd->onlinetime);
		//			strcpy(Account.SiteID,g_strHotelID);
		//			strcpy(Account.SiteName,g_strHotelName);
		//			strcpy(Account.ServType,row[8]);
		//			strcpy(Account.ComputerIp,row[5]);
		//			strcpy(Account.ComputerMac,row[6]);
		//			strcpy(Account.ComputerName,row[1]);
		//			strcpy(Account.Key1,row[10]);
		//			strcpy(Account.Key2,row[7]);
		//			strcpy(Account.DestIp,row[11]);
		//			Account.DateTime=GetDBTime(row[9]);
		//			strcpy(Account.ClientName,row[4]);
		//			strcpy(Account.CertType,row[2]);
		//			strcpy(Account.CertNo,row[3]);

	strncpy(ac.Key1, na->data.c_str(), 60);
	strncpy(ac.Key2, na->passwd.c_str(), sizeof(ac.Key2));

	snprintf(ac.DestIp, sizeof(ac.DestIp), "%03d.%03d.%03d.%03d",
			((u_char*) &(na->dstip))[0], ((u_char*) &(na->dstip))[1],
			((u_char*) &(na->dstip))[2], ((u_char*) &(na->dstip))[3]);

	snprintf(ac.Port, sizeof(ac.Port), "%d", na->dport);

#ifdef DEBUG
	std::cout << "场所编号: " << ac.SiteID << std::endl;
	std::cout << "帐号类型: " << ac.ServType << std::endl;
	std::cout << "帐号:" << ac.Key1 << std::endl;
	std::cout << "密码:" << ac.Key2 << std::endl;
	std::cout << "主机:" << ac.DestIp;
	std::cout << "用户名:" << pcd->CustomerName << std::endl;
	std::cout << "ID号码:" << ac.CertNo << std::endl;
	std::cout << "机器IP:" << ac.ComputerIp << std::endl;
#endif
	SendData(COMMAND_ACCOUNT, (char *) &ac, sizeof(ac));
}
void RecordAccout(struct CustomerData & cd)
{
	SendData(COMMAND_CUSTOMER, (char *) &cd, sizeof(CustomerData));
}

void RecordNetAccount(std::string & pType,std::string & Log,in_addr_t ip)
{
    std::string strTime;
    CString strSQL;
    struct Clients_DATA cd;
    struct Clients_DATA *pcd=&cd;
    in_addr in_addr_ip;
    in_addr_ip.s_addr = ip;

    if (get_client_data(ip, pcd) != 0)
		return;

    formattime(strTime);

	strSQL.Format(
			"insert into t_netlog (RoomNum,MachineIP,MachineMac,CustomerIDType,CustomerIDNum, "
				"CustomerName,nLogType,strLogInfo,nTime) values ('%s','%s','%s','%s','%s','%s','%s','%s','%s')",
			pcd->RoomNum.c_str(), inet_ntoa(in_addr_ip), pcd->MAC_ADDR,
			pcd->CustomerIDType.c_str(), pcd->CustomerID.c_str(),
			pcd->CustomerName.c_str(), pType.c_str(), Log.c_str(),
			strTime.c_str());
    ksql_run_query(strSQL);
}


int InitRecordSQL(const std::string & passwd, const std::string & user,
		const std::string & database, const std::string & host)
{
	MYSQL_RES *res;
    MYSQL_ROW row;
    pthread_mutexattr_t mutex_attr;

    log_printf(L_DEBUG_OUTPUT, "连接到数据库...");

 	mysql_init(&mysql);

 	pthread_mutex_destroy(&sql_mutex);

 	pthread_mutexattr_init(&mutex_attr);
 	pthread_mutexattr_settype(&mutex_attr,PTHREAD_MUTEX_RECURSIVE);

 	pthread_mutex_init(&sql_mutex,&mutex_attr);
 	pthread_mutexattr_destroy(&mutex_attr);

 	pthread_mutex_lock(&sql_mutex);

// 	mysql_set_character_set(&mysql,"utf-8");

    if(!mysql_real_connect(&mysql,host.c_str(),user.c_str(),passwd.c_str(),database.c_str(), 0, 0, 0))
    {
    	pthread_mutex_unlock(&sql_mutex);
        std::cerr << "Error connecting to database:"
                << mysql_error(&mysql) << std::endl;
        mysql_close(&mysql);
        return mysql_errno(&mysql);
    }
    mysql_set_character_set(&mysql,"utf8");
    pthread_mutex_unlock(&sql_mutex);
    log_printf(L_DEBUG_OUTPUT, "OK\n");

    //Get t_sysparam
	res = (MYSQL_RES*) ksql_query_and_use_result("select * from t_sysparam");
	row = ksql_fetch_row(res);
	if (row)
	{

		strcpy(hotel::strServerIP, row[1]);
//		strcpy(hotel::strServerIP, "192.168.50.168");
		strcpy(hotel::strHotelID, row[2]);
		strcpy(hotel::str_ethID + 3, row[3]);

		utf8_gbk(hotel::strHoteName,sizeof(hotel::strHoteName),row[4],strlen(row[4]));

		strcpy(hotel::strWebIP, row[5]);



		log_printf(L_DEBUG_OUTPUT,"hotel name is %s\n",row[4]);
		log_printf(L_DEBUG_OUTPUT,"hotel ID is %s\n",hotel::strHotelID);
		log_printf(L_DEBUG_OUTPUT,"ServerIP is %s\n",hotel::strServerIP);
		log_printf(L_DEBUG_OUTPUT,"WebIP is %s\n",hotel::strWebIP);
		log_printf(L_DEBUG_OUTPUT,"monitoring card %s\n",hotel::str_ethID);

	}
	ksql_free_result(res);

	//初始化跳转页面
	{
		CString dest;
		dest.Format("%s/login", hotel::strWebIP);

		init_http_redirector(std::string(dest));


	}
	//进行判断是否为旧式数据库格式
	log_printf(L_ERROR,"检查数据库格式为......");

	if(mysql_query( &mysql,"select * from room_list"))
	{
		ksql_free_result(res);
		hotel::Is_Old_DB = true;
//		sleep(1);
		log_printf(L_ERROR,"老数据库格式\t退出\n");

		return -1;
		// 老旧的上网数据库。
	}else
	{
		hotel::Is_Old_DB = false;
		log_printf(L_ERROR,"新数据库格式\tOK\n");
	}
	mysql_free_result(mysql_use_result(&mysql));

    return 0;
}

void ksql_close()
{
	mysql_close(&mysql);
}

bool ksql_is_server_gone()
{
	char	SQL_NULL[]="SELECT t.`nIndex` FROM t_sysparam t LIMIT 0,0";
	pthread_mutex_lock(&sql_mutex);
	if(mysql_query(&mysql, SQL_NULL))
	{
		pthread_mutex_unlock(&sql_mutex);
		return true;
	}
	else
	{
		int ret = mysql_field_count(&mysql);
		if(ret)
		{
			MYSQL_RES* res =  mysql_use_result(&mysql);
			if(res)
				mysql_free_result(res);
		}
		pthread_mutex_unlock(&sql_mutex);
	}
	return false;
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
	}
}
void* * ksql_query_and_use_result(const char* query)
{
	MYSQL_RES *	res;

	pthread_mutex_lock(&sql_mutex);

	if(mysql_query(&mysql,query))
	{
		pthread_mutex_unlock(&sql_mutex);
		log_printf(L_ERROR,"Err make query  %s\n",mysql_error(&mysql));
		if(mysql_errno(&mysql)==CR_SERVER_GONE_ERROR)
		{
			close(open("/tmp/monitor.socket",O_RDWR));
		}
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

void ksql_thread_init()
{
	mysql_thread_init();
}

void ksql_thread_end()
{
	mysql_thread_end();
}

void InsertCustomerLog(const char * build,const char * floor,const char * room, const char * name ,
		const char * idtype , const char * id, const char * type,const char * ip, const char * mac, const char * time)
{
	CString sqlstr;

	sqlstr.Format("insert into t_customerlog ("
			"BuildNum,RoomFloor,RoomNum,CustomerName,CustomerIDType,"
			"CustomerIDNum,MachineIP,MachineMac,nType,HappyTime) "
			"values ('%s','%s','%s','%s','%s','%s','%s','%s','%s','%s')",
			build,floor,room,name,idtype,id,ip,mac,type,time);
	//log_printf(L_DEBUG_OUTPUT,"%s\n",sqlstr.c_str());
	ksql_run_query(sqlstr);
}
