/*
 * ksql_static_template.h
 *
 *      Copyright 2009-2010 薇菜工作室
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *      MA 02110-1301, USA.
 */

#ifndef SQLITE_STATIC_TEMPLATE_H_
#define SQLITE_STATIC_TEMPLATE_H_


static const char * create_sql[] =
{
//				--
//				-- 表的结构 `roomer_list`
//				--
		"CREATE TABLE IF NOT EXISTS `roomer_list` ("
		"  `nIndex` INTEGER PRIMARY KEY AUTOINCREMENT," //COMMENT \'key,自动增长\',"
		"  `LoginNum` TEXT DEFAULT NULL," // COMMENT \'登录号\',"
		"  `CustomerName` varchar(20) DEFAULT NULL,"
		"  `IDtype` int(11) NOT NULL DEFAULT 1 ," //COMMENT \' 证件类型\',"
		"  `ID` varchar(32) NOT NULL DEFAULT 0 ," //COMMENT \'证件号码\',"
		"  `IP_ADDR` TEXT DEFAULT NULL," // COMMENT \'ip 地址   （由登录页面添加，如果是白名单就直接添加）\',"
		"  `MAC_ADDR` char(19) DEFAULT NULL," // COMMENT \'MAC地址  （xx:xx:xx:xx:xx:xx 格式）（由本程序自动填写）\',"
		"  `RoomId` int(32) DEFAULT 0,"
		"  `Time` datetime DEFAULT NULL," // COMMENT \'客户登记时间 （登记的时候添加）\',"
		"  `LoginTime` datetime DEFAULT NULL," // COMMENT \'客户开始登录上网的时间 （客户登录的时候添加）\',"
		"  `OnlineTime` datetime DEFAULT NULL," // COMMENT \'客户实际开始访问网络的时间 (本程序维护)\',"
		"  `live_address` varchar(30) NOT NULL DEFAULT \'\' ," //COMMENT \'客户住址。\',"
		"  `country` varchar(3) NOT NULL DEFAULT \'CHN\'," // COMMENT \'国籍\',"
		"  `org` varchar(70) NOT NULL DEFAULT \'公安部\' ," //COMMENT \'发证机关\',"
		"  `IsDelete` tinyint(4) NOT NULL DEFAULT \'0\'"
		") ",


//				--
//				-- 表的结构 `room_list`
//				--
		"CREATE TABLE IF NOT EXISTS `room_list` ("
		"  `nIndex` INTEGER PRIMARY KEY AUTOINCREMENT,"
		"  `RoomBuild` int(11) NOT NULL DEFAULT 0,"
		"  `RoomFloor` int(11) NOT NULL DEFAULT 0,"
		"  `RoomNum` int(11) NOT NULL DEFAULT 0,"
		"  `MAC_ADDR` TEXT DEFAULT NULL,"
		"  `IsBind` tinyint(4) NOT NULL DEFAULT \'0\',"
		"  `RoomerCount` int(11) NOT NULL DEFAULT \'0\'"
		") ",

//				--
//				-- 表的结构 `t_customerlog`
//				--
		"CREATE TABLE IF NOT EXISTS `t_customerlog` (\n"
		"  `nIndex` INTEGER PRIMARY KEY AUTOINCREMENT,"
		"  `BuildNum` varchar(8) NOT NULL DEFAULT 0,\n"
		"  `RoomFloor` int(11) NOT NULL DEFAULT 0,\n"
		"  `RoomNum` varchar(20) NOT NULL DEFAULT 0,\n"
		"  `CustomerName` varchar(20) DEFAULT NULL,\n"
		"  `CustomerIDType` int(11) NOT NULL DEFAULT 0,\n"
		"  `CustomerIDNum` varchar(40)  DEFAULT NULL,\n"
		"  `MachineIP` varchar(20) DEFAULT NULL,\n"
		"  `MachineMac` varchar(20) DEFAULT NULL,\n"
		"  `nType` int(11) NOT NULL DEFAULT \'0\' ,"//COMMENT \'入住是0，退房是1\',\n"
		"  `HappyTime` timestamp NULL DEFAULT NULL\n"
		") ",

//				--
//				-- 表的结构 `t_floor`
//				--

		"CREATE TABLE IF NOT EXISTS `t_floor` (\n"
		"  `nIndex` INTEGER PRIMARY KEY AUTOINCREMENT,"
		"  `floor` int(11) NOT NULL DEFAULT \'0\',\n"
		"  `nbuild` tinyint(3) NOT NULL DEFAULT \'0\'\n"
		") ",

//				--
//				-- 表的结构 `t_netlog`
//				--
		"CREATE TABLE IF NOT EXISTS `t_netlog` (\n"
		"  `nIndex` INTEGER PRIMARY KEY AUTOINCREMENT,\n"
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
		"  `sReserve3` varchar(50) DEFAULT NULL\n"
		") ",

//				--
//				-- 表的结构 `t_sysparam`
//				--

		"CREATE TABLE IF NOT EXISTS `t_sysparam` (\n"
		"  `nIndex` INTEGER PRIMARY KEY AUTOINCREMENT,\n"
		"  `HOSTIP` varchar(20) NOT NULL DEFAULT \'\',\n"
		"  `HotelID` varchar(15) NOT NULL,\n"
		"  `NetAdapter` int(4) NOT NULL DEFAULT \'0\',\n"
		"  `HotelName` varchar(20) NOT NULL DEFAULT \'\',\n"
		"  `WEBSERVERIP` varchar(20) NOT NULL DEFAULT \'\',\n"
		"  `EngHotelName` varchar(50) DEFAULT NULL\n"
		") ",

//				--
//				-- 表的结构 `whitelist`
//				--

		"CREATE TABLE IF NOT EXISTS `whitelist` (\n"
		"  `nIndex` INTEGER PRIMARY KEY AUTOINCREMENT,\n"
		"  `MAC_ADDR` TEXT DEFAULT NULL,\n"
		"  `CustomerName` TEXT DEFAULT NULL,\n"
		"  `IDtype` int(11) DEFAULT NULL,\n"
		"  `ID` TEXT DEFAULT NULL"
		") "
};


#endif /* KSQL_STATIC_TEMPLATE_H_ */
