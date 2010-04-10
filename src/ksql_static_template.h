/*
 * ksql_static_template.h
 *
 *  Created on: 2010-4-11
 *      Author: cai
 */

#ifndef KSQL_STATIC_TEMPLATE_H_
#define KSQL_STATIC_TEMPLATE_H_


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


#endif /* KSQL_STATIC_TEMPLATE_H_ */
