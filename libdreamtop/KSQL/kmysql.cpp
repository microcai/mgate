

/*
 * 	mysql.cpp
 *
 * 	Copyright 2009 microcai(microcai@sina.com)
 *
 *	microcai's Wrapper for MySQL client lib
 *
 */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <iostream>
#include <queue>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <fcntl.h>
#include <syscall.h>
#include <pthread.h>
#include <syslog.h>
#include <sys/poll.h>

#include <netinet/in.h>
#include <netinet/ip.h>
#include <net/ethernet.h>
#include <mysql/mysql.h>
#include <mysql/errmsg.h>

#include <sqlite3.h>

#define		__KLIBSQL_USEINTERNALLY
#include "libdreamtop.h"
#include "kmysql.h"

#define __PACKED__

KSQL_RES* ksql_query_and_use_result(const char* query);

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
		") ENGINE=MyISAM  DEFAULT CHARSET=utf8",

		"\0"
};

int InitMySQL(const std::string & passwd, const std::string & user,
		const std::string & database, const std::string & host)
{
    pthread_mutexattr_t mutex_attr;

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
        	syslog(LOG_ERR,"database %s does not exsit, create it.",database.c_str());

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

    syslog(LOG_NOTICE,"连接到数据库 OK\n");
    return 0;
}

void kmysql_close()
{
	mysql_close(&mysql);
}

bool is_mysqlserver_gone()
{
	char SQL_NULL[] = "SELECT t.`nIndex` FROM t_sysparam t LIMIT 0,0";
	pthread_mutex_lock(&sql_mutex);
	if (mysql_query(&mysql, SQL_NULL))
	{
		pthread_mutex_unlock(&sql_mutex);
		return true;
	}
	else
	{
		int ret = mysql_field_count(&mysql);
		if (ret)
		{
			MYSQL_RES* res = mysql_use_result(&mysql);
			if (res)
				mysql_free_result(res);
		}
		pthread_mutex_unlock(&sql_mutex);
	}
	return false;
}

int kmysql_run_query(const char *p)
{
	pthread_mutex_lock(&sql_mutex);
	int ret = mysql_query(&mysql, p);
	pthread_mutex_unlock(&sql_mutex);
	//尽量减少加锁的时间，增加并行。
	if (ret)
		syslog(LOG_ERR, "err make query  %s\n", mysql_error(&mysql));
	return ret;
}

KSQL_ROW kmysql_fetch_row(KSQL_RES*res)
{
	if(res)
		return mysql_fetch_row((MYSQL_RES*)res);
	else
		return NULL;
}

void kmysql_free_result(KSQL_RES* res)
{
	if (res)
	{
		mysql_free_result((MYSQL_RES*) res);
	}
}

KSQL_RES* kmysql_query_and_use_result(const char* query)
{
	MYSQL_RES * res;
	pthread_mutex_lock(&sql_mutex);

	if (mysql_query(&mysql, query))
	{
		pthread_mutex_unlock(&sql_mutex);
		syslog(LOG_ERR, "Err make query  %s\n", mysql_error(&mysql));
		if (mysql_errno(&mysql) == CR_SERVER_GONE_ERROR)
		{
			close(open("/tmp/monitor.socket", O_RDWR));
		}
		return NULL;
	}
	res = mysql_store_result(&mysql);

	pthread_mutex_unlock(&sql_mutex);

	return (KSQL_RES*) res;
}

int InitMysqlDB()
{
	for (int i = 0; i < (int) (sizeof(create_sql) / sizeof(char*)); ++i)
		mysql_query(&mysql, create_sql[i]);
	return 0;
}

