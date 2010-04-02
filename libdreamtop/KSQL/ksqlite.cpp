/*
 * ksqlite.cpp
 *
 *  Created on: 2009-10-21
 *      Author: cai
 */

#include <string>
#include <stdlib.h>
#include <syslog.h>
#include <sqlite3.h>
#include <glib.h>
#include "libdreamtop.h"
#include "ksqlite.h"

static const char * create_sql[] =
{
//			--
//			-- 表的结构 `portmap`
//			--
		"CREATE TABLE IF NOT EXISTS `portmap` ("
		"  `nIndex` int(32) NOT NULL,"
		"  `protocol` TEXT DEFAULT NULL ,"
		"  `wport` int(11) DEFAULT NULL ,"
		"  `lport` int(11) DEFAULT NULL ,"
		"  `IP_ADDR` varchar(32)  DEFAULT NULL,"
		"  `WAN_IP` varchar(32) DEFAULT('0.0.0.0'),"
		"  PRIMARY KEY (`nIndex`)"
		")",

//				--
//				-- 表的结构 `portmap_change`
//				--

		"CREATE TABLE IF NOT EXISTS `portmap_change` (\n"
		"  `nIndex` int(32) NOT NULL ,\n"
		"  `Is_ADD` int(4) DEFAULT NULL ,\n"
		"  `protocol` TEXT  DEFAULT NULL,\n"
		"  `wport` int(11) DEFAULT NULL ,"
		"  `lport` int(11) DEFAULT NULL ,"
		"  `IP_ADDR` TEXT DEFAULT NULL ,"
		"  `WAN_IP` TEXT DEFAULT NULL,\n"
		"  PRIMARY KEY (`nIndex`)\n"
		")",

//				--
//				-- 表的结构 `roomer_list`
//				--
		"CREATE TABLE IF NOT EXISTS `roomer_list` ("
		"  `nIndex` int(32) NOT NULL,"
		"  `LoginNum` TEXT DEFAULT NULL,"
		"  `CustomerName` TEXT DEFAULT NULL,"
		"  `IDtype` int(11) NOT NULL DEFAULT 1 ,"
		"  `ID` varchar(32) NOT NULL DEFAULT 0 ,"
		"  `IP_ADDR` TEXT  DEFAULT NULL,"
		"  `MAC_ADDR` TEXT DEFAULT NULL,"
		"  `RoomId` int DEFAULT 0,"
		"  `Time` datetime DEFAULT NULL ,"
		"  `LoginTime` datetime DEFAULT NULL ,"
		"  `OnlineTime` datetime DEFAULT NULL,"
		"  `live_address` varchar(30) NOT NULL DEFAULT ('') ,"
		"  `country` varchar(3) NOT NULL DEFAULT ('CHN') ,"
		"  `org` varchar(70) NOT NULL DEFAULT ('公安部') ,"
		"  `IsDelete` int(4) NOT NULL DEFAULT ('0') ,"
		"  PRIMARY KEY (`nIndex`)"
		")",

//				--
//				-- 表的结构 `room_change`
//				--
		"CREATE TABLE IF NOT EXISTS `room_change` ("
		"  `nIndex` int(32) NOT NULL ,"
		"  `RoomerId` int(32) NOT NULL DEFAULT 0,"
		"  `ActionType` int(11) NOT NULL DEFAULT 3,"
		"  PRIMARY KEY (`nIndex`)"
		")",

//				--
//				-- 表的结构 `room_list`
//				--
		"CREATE TABLE IF NOT EXISTS `room_list` ("
		"  `nIndex` int(32) NOT NULL ,"
		"  `RoomBuild` int(11) NOT NULL DEFAULT 0,"
		"  `RoomFloor` int(11) NOT NULL DEFAULT 0,"
		"  `RoomNum` int(11) NOT NULL DEFAULT 0,"
		"  `MAC_ADDR` char(19) DEFAULT NULL,"
		"  `IsBind` int(4) NOT NULL DEFAULT \'0\',"
		"  `RoomerCount` int(11) NOT NULL DEFAULT \'0\',"
		"  PRIMARY KEY (`nIndex`)"
		")",

//				--
//				-- 表的结构 `t_customerlog`
//				--
		"CREATE TABLE IF NOT EXISTS `t_customerlog` (\n"
		"  `nIndex` int NOT NULL ,\n"
		"  `BuildNum` varchar(8) NOT NULL DEFAULT 0,\n"
		"  `RoomFloor` int(11) NOT NULL DEFAULT 0,\n"
		"  `RoomNum` varchar(20) NOT NULL DEFAULT 0,\n"
		"  `CustomerName` varchar(20) DEFAULT NULL,\n"
		"  `CustomerIDType` int(11) NOT NULL DEFAULT 0,\n"
		"  `CustomerIDNum` varchar(40)  DEFAULT NULL,\n"
		"  `MachineIP` varchar(20) DEFAULT NULL,\n"
		"  `MachineMac` varchar(20) DEFAULT NULL,\n"
		"  `nType` int(11) NOT NULL DEFAULT \'0\' ,\n"
		"  `HappyTime` timestamp NULL DEFAULT NULL,\n"
		"  PRIMARY KEY (`nIndex`)\n"
		")",

//				--
//				-- 表的结构 `t_floor`
//				--

		"CREATE TABLE IF NOT EXISTS `t_floor` (\n"
		"  `nIndex` int(11) NOT NULL AUTO_INCREMENT,\n"
		"  `floor` int(11) NOT NULL DEFAULT \'0\',\n"
		"  `nbuild` int(3) unsigned NOT NULL DEFAULT \'0\',\n"
		"  PRIMARY KEY (`nIndex`)\n"
		") ",

//				--
//				-- 表的结构 `t_netlog`
//				--
		"CREATE TABLE IF NOT EXISTS `t_netlog` (\n"
		"  `nIndex` int(11) NOT NULL ,\n"
		"  `RoomNum` varchar(20) NOT NULL DEFAULT \'\',\n"
		"  `MachineIP` varchar(20) NOT NULL DEFAULT \'\',\n"
		"  `MachineMac` varchar(20) NOT NULL DEFAULT \'\',\n"
		"  `CustomerIDType` varchar(4) NOT NULL DEFAULT \'0\',\n"
		"  `CustomerIDNum` varchar(40) NOT NULL DEFAULT \'\',\n"
		"  `CustomerName` varchar(20) NOT NULL DEFAULT \'\',\n"
		"  `nLogType` varchar(4) NOT NULL DEFAULT \'0\',\n"
		"  `strLogInfo` varchar(50) NOT NULL DEFAULT \'\',\n"
		"  `nTime` datetime NOT NULL DEFAULT '0000-00-00 00:00:00',\n"
		"  `sReserve1` varchar(100) DEFAULT NULL,\n"
		"  `sReserve2` varchar(100) DEFAULT NULL,\n"
		"  `sReserve3` varchar(50) DEFAULT NULL,\n"
		"  PRIMARY KEY (`nIndex`)\n"
		") ",

//				--
//				-- 表的结构 `t_sysparam`
//				--

		"CREATE TABLE IF NOT EXISTS `t_sysparam` ("
		"  `nIndex` INTEGER DEFAULT ('NULL'),"
		"  `HOSTIP` TEXT NOT NULL,"
		"  `HotelID` TEXT NOT NULL,"
		"  `NetAdapter` INTEGER NOT NULL DEFAULT (1),"
		"  `HotelName` EXT NOT NULL,"
		"  `WEBSERVERIP` EXT NOT NULL,"
		"  `EngHotelName` EXT NOT NULL"
		")",

//				--
//				-- 表的结构 `users`
//				--
		"CREATE TABLE IF NOT EXISTS `users` ("
		"  `id` tinyint(10) NOT NULL ,"
		"  `user` TEXT NOT NULL,"
		"  `password` TEXT NOT NULL,"
		"  `status` TEXT NOT NULL,"
		"  PRIMARY KEY (`id`)"
		") ",

//				--
//				-- 导出表中的数据 `users`
//				--
		"insert into users (user,password,status) values ('admin','admin',0)",

//				--
//				-- 表的结构 `whitelist`
//				--

		"CREATE TABLE IF NOT EXISTS `whitelist` (\n"
		"  `nIndex` int(32) NOT NULL ,\n"
		"  `MAC_ADDR` char(19) DEFAULT NULL,\n"
		"  `CustomerName` TEXT DEFAULT NULL,\n"
		"  `IDtype` int(11) DEFAULT NULL,\n"
		"  `ID` varchar(32) DEFAULT NULL,\n"
		"  PRIMARY KEY (`nIndex`)\n"
		") ",

		"\0"
};

static sqlite3 *sqldb;

void InitSqlite()
{
	sqlite3_initialize();

	const gchar * sqlite_db;

	extern GKeyFile *  gkeyfile;

	sqlite_db = g_key_file_get_string(gkeyfile,"sqlite","db.sqlite",NULL);
	if (!sqlite_db)
		sqlite_db = "/var/lib/monitor.sqlite";
	if (sqlite3_open(sqlite_db, &sqldb) != SQLITE_OK)
	{
		syslog(LOG_ERR, "open db '%s' failed", sqlite_db);
	}
}

void InitSqliteDB()
{
	for (int i = 0; i < (int) (sizeof(create_sql) / sizeof(char*)); ++i)
		ksqlite_query(create_sql[i]);
}

int ksqlite_query(const char * q)
{
	char * err;
	int ret;
	ret = sqlite3_exec(sqldb,q,0,0,&err);
	return ret;
}

KSQL_RES * ksqlite_query_and_use_result(const char* query)
{
	struct
	{
		KSQL_ROW row;
		int nRow;
		int nColumn;
		char *err;
		int currentrow;
	}*ksql_res;
	ksql_res = (typeof(ksql_res)) malloc(sizeof(*ksql_res));

	if (sqlite3_get_table(sqldb, query, &ksql_res->row, &ksql_res->nRow,
			&ksql_res->nColumn, &ksql_res->err))
	{
		free(ksql_res);
		return NULL;
	}
	else
	{
		ksql_res->currentrow = 0;
		return (KSQL_RES*) ksql_res;
	}
}

KSQL_ROW ksqlite_fetch_row(KSQL_RES*res)
{
	struct
	{
		KSQL_ROW row;
		int nRow;
		int nColumn;
		char *err;
		int currentrow;
	}*ksql_res = (typeof(ksql_res)) res;

	if(res){

	ksql_res->currentrow++;
	if (ksql_res->currentrow > ksql_res->nRow)
		return NULL;
	return & ksql_res->row[ksql_res->currentrow * ksql_res->nColumn];
	}else return NULL;
}

void ksqlite_free_result(KSQL_RES* res)
{
	struct
	{
		KSQL_ROW row;
		int nRow;
		int nColumn;
		char *err;
		int currentrow;
	}*ksql_res = (typeof(ksql_res)) res;

	sqlite3_free_table(ksql_res->row);
	free(res);
}
