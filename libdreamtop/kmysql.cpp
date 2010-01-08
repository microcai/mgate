

/*
 * 	mysql.cpp
 *
 * 	Copyright 2009 microcai(microcai@sina.com)
 *
 *	microcai's Wrapper for MySQL client lib
 *
 */

#include <iostream>
//#include <map>
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

static const char * create_sql[] =
{
//			--
//			-- 表的结构 `portmap`
//			--
		"CREATE TABLE IF NOT EXISTS `portmap` ("
		"  `nIndex` int(32) NOT NULL AUTO_INCREMENT,"
		"  `protocol` varchar(255) CHARACTER SET utf8 DEFAULT NULL COMMENT 'tcp or udp',"
		"  `wport` int(11) DEFAULT NULL COMMENT \'外部端口\',"
		"  `lport` int(11) DEFAULT NULL COMMENT \'内部端口\',"
		"  `IP_ADDR` varchar(32) CHARACTER SET utf8 DEFAULT NULL COMMENT '映射的IP',"
		"  `WAN_IP` varchar(32) CHARACTER SET utf8 DEFAULT '0.0.0.0' COMMENT '外部网络 IP (对于拥有多个外网 IP 的主机，如果希望不是映射所有的外网地址上的端口到局域网内部电脑需要填写本项目，否者为 0.0.0.0)',"
		"  PRIMARY KEY (`nIndex`)"
		") ENGINE=MyISAM  DEFAULT CHARSET=utf8",

//				--
//				-- 表的结构 `portmap_change`
//				--

		"CREATE TABLE IF NOT EXISTS `portmap_change` (\n"
		"  `nIndex` int(32) NOT NULL AUTO_INCREMENT,\n"
		"  `Is_ADD` tinyint(4) DEFAULT NULL COMMENT \'1 表示有新的端口映射要插入， 0 表示要删除\',\n"
		"  `protocol` varchar(255) CHARACTER SET utf8 DEFAULT NULL COMMENT \'tcp or udp\',\n"
		"  `wport` int(11) DEFAULT NULL COMMENT \'外部端口\',"
		"  `lport` int(11) DEFAULT NULL COMMENT \'内部端口\',"
		"  `IP_ADDR` varchar(32) DEFAULT NULL COMMENT \'映射的IP\',"
		"  `WAN_IP` varchar(255) CHARACTER SET utf8 DEFAULT NULL COMMENT \'外部网络 IP (对于拥有多个外网 IP 的主机，如果希望不是映射所有的外网地址上的端口到局域网内部电脑需要填写本项目，否者为 0.0.0.0)\',\n"
		"  PRIMARY KEY (`nIndex`)\n"
		") ENGINE=MEMORY  DEFAULT CHARSET=utf8",

//				--
//				-- 表的结构 `roomer_list`
//				--
		"CREATE TABLE IF NOT EXISTS `roomer_list` ("
		"  `nIndex` int(32) NOT NULL AUTO_INCREMENT COMMENT \'key,自动增长\',"
		"  `LoginNum` varchar(20) CHARACTER SET utf8 COLLATE utf8_bin DEFAULT NULL COMMENT \'登录号\',"
		"  `CustomerName` varchar(20) DEFAULT NULL,"
		"  `IDtype` int(11) NOT NULL DEFAULT 1 COMMENT \' 证件类型\',"
		"  `ID` varchar(32) NOT NULL DEFAULT 0 COMMENT \'证件号码\',"
		"  `IP_ADDR` varchar(20) CHARACTER SET ascii DEFAULT NULL COMMENT \'ip 地址   （由登录页面添加，如果是白名单就直接添加）\',"
		"  `MAC_ADDR` char(19) DEFAULT NULL COMMENT \'MAC地址  （xx:xx:xx:xx:xx:xx 格式）（由本程序自动填写）\',"
		"  `RoomId` int(32) DEFAULT 0,"
		"  `Time` datetime DEFAULT NULL COMMENT \'客户登记时间 （登记的时候添加）\',"
		"  `LoginTime` datetime DEFAULT NULL COMMENT \'客户开始登录上网的时间 （客户登录的时候添加）\',"
		"  `OnlineTime` datetime DEFAULT NULL COMMENT \'客户实际开始访问网络的时间 (本程序维护)\',"
		"  `live_address` varchar(30) NOT NULL DEFAULT \'\' COMMENT \'客户住址。\',"
		"  `country` varchar(3) NOT NULL DEFAULT \'CHN\' COMMENT \'国籍\',"
		"  `org` varchar(70) NOT NULL DEFAULT \'公安部\' COMMENT \'发证机关\',"
		"  `IsDelete` tinyint(4) NOT NULL DEFAULT \'0\',"
		"  PRIMARY KEY (`nIndex`)"
		") ENGINE=MyISAM  DEFAULT CHARSET=utf8",

//				--
//				-- 表的结构 `room_change`
//				--
		"CREATE TABLE IF NOT EXISTS `room_change` ("
		"  `nIndex` int(32) NOT NULL AUTO_INCREMENT,"
		"  `RoomerId` int(32) NOT NULL DEFAULT 0,"
		"  `ActionType` int(11) NOT NULL DEFAULT 3,"
		"  PRIMARY KEY (`nIndex`)"
		")ENGINE=MEMORY  DEFAULT CHARSET=utf8",

//				--
//				-- 表的结构 `room_list`
//				--
		"CREATE TABLE IF NOT EXISTS `room_list` ("
		"  `nIndex` int(32) NOT NULL AUTO_INCREMENT,"
		"  `RoomBuild` int(11) NOT NULL DEFAULT 0,"
		"  `RoomFloor` int(11) NOT NULL DEFAULT 0,"
		"  `RoomNum` int(11) NOT NULL DEFAULT 0,"
		"  `MAC_ADDR` char(19) CHARACTER SET utf8 DEFAULT NULL,"
		"  `IsBind` tinyint(4) NOT NULL DEFAULT \'0\',"
		"  `RoomerCount` int(11) NOT NULL DEFAULT \'0\',"
		"  PRIMARY KEY (`nIndex`)"
		") ENGINE=MyISAM  DEFAULT CHARSET=utf8",

//				--
//				-- 表的结构 `t_customerlog`
//				--
		"CREATE TABLE IF NOT EXISTS `t_customerlog` (\n"
		"  `nIndex` int NOT NULL AUTO_INCREMENT,\n"
		"  `BuildNum` varchar(8) NOT NULL DEFAULT 0,\n"
		"  `RoomFloor` int(11) NOT NULL DEFAULT 0,\n"
		"  `RoomNum` varchar(20) NOT NULL DEFAULT 0,\n"
		"  `CustomerName` varchar(20) DEFAULT NULL,\n"
		"  `CustomerIDType` int(11) NOT NULL DEFAULT 0,\n"
		"  `CustomerIDNum` varchar(40)  DEFAULT NULL,\n"
		"  `MachineIP` varchar(20) DEFAULT NULL,\n"
		"  `MachineMac` varchar(20) DEFAULT NULL,\n"
		"  `nType` int(11) NOT NULL DEFAULT \'0\' COMMENT \'入住是0，退房是1\',\n"
		"  `HappyTime` timestamp NULL DEFAULT NULL,\n"
		"  PRIMARY KEY (`nIndex`)\n"
		") ENGINE=MyISAM  DEFAULT CHARSET=utf8",

//				--
//				-- 表的结构 `t_floor`
//				--

		"CREATE TABLE IF NOT EXISTS `t_floor` (\n"
		"  `nIndex` int(11) NOT NULL AUTO_INCREMENT,\n"
		"  `floor` int(11) NOT NULL DEFAULT \'0\',\n"
		"  `nbuild` tinyint(3) unsigned NOT NULL DEFAULT \'0\',\n"
		"  PRIMARY KEY (`nIndex`)\n"
		") ENGINE=MyISAM  DEFAULT CHARSET=utf8",

//				--
//				-- 表的结构 `t_netlog`
//				--
		"CREATE TABLE IF NOT EXISTS `t_netlog` (\n"
		"  `nIndex` int(11) NOT NULL AUTO_INCREMENT,\n"
		"  `RoomNum` varchar(20) NOT NULL DEFAULT \'\',\n"
		"  `MachineIP` varchar(20) NOT NULL DEFAULT \'\',\n"
		"  `MachineMac` varchar(20) NOT NULL DEFAULT \'\',\n"
		"  `CustomerIDType` varchar(4) NOT NULL DEFAULT \'0\',\n"
		"  `CustomerIDNum` varchar(40) NOT NULL DEFAULT \'\',\n"
		"  `CustomerName` varchar(20) NOT NULL DEFAULT \'\',\n"
		"  `nLogType` varchar(4) NOT NULL DEFAULT \'0\',\n"
		"  `strLogInfo` varchar(50) NOT NULL DEFAULT \'\',\n"
		"  `nTime` datetime NOT NULL DEFAULT \'0000-00-00 00:00:00\',\n"
		"  `sReserve1` varchar(100) DEFAULT NULL,\n"
		"  `sReserve2` varchar(100) DEFAULT NULL,\n"
		"  `sReserve3` varchar(50) DEFAULT NULL,\n"
		"  PRIMARY KEY (`nIndex`)\n"
		") ENGINE=MyISAM DEFAULT CHARSET=utf8",

//				--
//				-- 表的结构 `t_sysparam`
//				--

		"CREATE TABLE IF NOT EXISTS `t_sysparam` (\n"
		"  `nIndex` int(11) NOT NULL AUTO_INCREMENT,\n"
		"  `HOSTIP` varchar(20) NOT NULL DEFAULT \'\',\n"
		"  `HotelID` varchar(15) NOT NULL,\n"
		"  `NetAdapter` int(4) NOT NULL DEFAULT \'0\',\n"
		"  `HotelName` varchar(20) NOT NULL DEFAULT \'\',\n"
		"  `WEBSERVERIP` varchar(20) NOT NULL DEFAULT \'\',\n"
		"  `EngHotelName` varchar(50) DEFAULT NULL,\n"
		"  PRIMARY KEY (`nIndex`)\n"
		") ENGINE=MyISAM  DEFAULT CHARSET=utf8",

//				--
//				-- 表的结构 `users`
//				--
		"CREATE TABLE IF NOT EXISTS `users` ("
		"  `id` tinyint(10) NOT NULL AUTO_INCREMENT,"
		"  `user` varchar(10) NOT NULL DEFAULT \'\',"
		"  `password` varchar(10) NOT NULL DEFAULT \'\',"
		"  `status` varchar(50) NOT NULL DEFAULT \'\',"
		"  PRIMARY KEY (`id`)"
		") ENGINE=MyISAM  DEFAULT CHARSET=utf8",

//				--
//				-- 导出表中的数据 `users`
//				--
		"insert into users (user,password,status) values ('admin','admin',0)",

//				--
//				-- 表的结构 `whitelist`
//				--

		"CREATE TABLE IF NOT EXISTS `whitelist` (\n"
		"  `nIndex` int(32) NOT NULL AUTO_INCREMENT,\n"
		"  `MAC_ADDR` char(19) CHARACTER SET utf8 DEFAULT NULL,\n"
		"  `CustomerName` varchar(20) CHARACTER SET utf8 DEFAULT NULL,\n"
		"  `IDtype` int(11) DEFAULT NULL,\n"
		"  `ID` varchar(32) CHARACTER SET utf8 DEFAULT NULL,\n"
		"  PRIMARY KEY (`nIndex`)\n"
		") ENGINE=MyISAM  DEFAULT CHARSET=utf8"

};

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

	u_char * mac = (u_char*)( na->packet + 6);

	if (get_client_data(mac,pcd)!=0) // there is no ..... so, let's just ignore it.
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

	strncpy(ac.CertType, pcd->CustomerIDType.c_str(),sizeof(ac.CertType)-1);
	strncpy(ac.CertNo, pcd->CustomerID.c_str(),sizeof(ac.CertNo)-1);

	utf8_gbk(ac.ClientName ,PROLEN_CLIENTNAME, pcd->CustomerName.c_str(),pcd->CustomerName.length());

	snprintf(ac.ComputerIp, sizeof(ac.ComputerIp)-1, "%03d.%03d.%03d.%03d",
			((u_char*) &(pcd->ip))[0], ((u_char*) &(pcd->ip))[1],
			((u_char*) &(pcd->ip))[2], ((u_char*) &(pcd->ip))[3]);

	MAC_ADDR2macaddr(ac.ComputerMac, pcd->MAC_ADDR);

	snprintf(ac.ComputerName,sizeof(ac.ComputerName),"%c%c%02d",
			*pcd->Build.c_str(),*pcd->Floor.c_str(), atoi(pcd->RoomNum.c_str()));

	strcpy(ac.ServType, na->strType);

	strSQL.Format(SQL_template, pcd->Build.c_str(),pcd->Floor.c_str(),atoi(pcd->RoomNum.c_str()), pcd->ip_addr.c_str(),
			pcd->mac_addr.c_str(), pcd->CustomerIDType.c_str(),
			pcd->CustomerID.c_str(), pcd->CustomerName.c_str(), na->strType,
			na->data.c_str(), strTime.c_str());
	ksql_run_query(strSQL);

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
	std::cout << "用户名:" << pcd->CustomerName << std::endl;
	std::cout << "机器IP:" << ac.ComputerIp << std::endl;
#endif
	SendData(COMMAND_ACCOUNT, (char *) &ac, sizeof(ac));
}
void RecordAccout(struct CustomerData & cd)
{
	SendData(COMMAND_CUSTOMER, (char *) &cd, sizeof(CustomerData));
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

    if(!mysql_real_connect(&mysql,host.c_str(),user.c_str(),passwd.c_str(),database.c_str(), 0, 0, 0))
    {

        if(mysql_errno(&mysql)==1049)
        {
        	log_printf(L_WARNING,"database %s does not exsit, create it.",database.c_str());

        	// 在这里重新建立数据库
        	if (mysql_real_connect(&mysql, host.c_str(), user.c_str(),
					passwd.c_str(), NULL, 0, 0, 0))
			{
        		CString sql;
        		sql.Format("create database %s",database.c_str());
        		mysql_query(&mysql,sql);
        		mysql_select_db(&mysql,database.c_str());
			}
			else goto ERR;
		}
		else
		{
			std::cerr << "Error connecting to database:" << mysql_error(&mysql)
				<< std::endl;
			ERR: mysql_close(&mysql);
			pthread_mutex_unlock(&sql_mutex);
			return mysql_errno(&mysql);
		}
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

	}else
	{

    	log_printf(L_WARNING, "tables not exist, create them.\n");
		for (int i = 0; i < (int)(sizeof(create_sql) / sizeof(char*)); ++i)
			mysql_query(&mysql, create_sql[i]);
		std::cerr << "All tables created!" << std::endl;
		exit(0);
	}
	ksql_free_result(res);

	//初始化跳转页面
	CString dest;
	dest.Format("%s/login", hotel::strWebIP);

	init_http_redirector(std::string(dest));

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
	int ret = mysql_query(&mysql, p );
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
