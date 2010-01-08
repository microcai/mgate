/*
 * kroom.cpp
 *
 *  Created on: 2009-5-8
 *      Author: cai
 *
 * kroom 是负责管理客房的网络访问的。包括上下网和端口影映射
 * iptables代码来自filter。
 *
 */

#include <iostream>
#include <stdio.h>
#include <stdlib.h>

#include "libmicrocai.h"
#include "kmysql.h"

static int nat_helper_event;
static bool	Is_Old_DB=false;

static void loadroomlist(MYSQL_ROW row,void*)
{
	struct Clients_DATA cd(row[3]); // 初始化的就是IP地址
	char macaddr[20];
	cd.CustomerName = row[0];
	cd.CustomerIDType = row[1];
	cd.CustomerID = row[2];
	cd.Build = row[4];
	cd.Floor = row[5];
	cd.RoomNum = row[6];
	if (GetMac(row[3], macaddr, cd.MAC_ADDR))
	{
		char sql[1024];
		cd.mac_addr = macaddr;
		if(Is_Old_DB)
			sprintf(sql,"update t_room_list set MachineMac=\'%s\' where nIndex=%s",cd.mac_addr.c_str(),row[7]);
		else
			sprintf(sql,"update room_list set MAC_ADDR=\'%s\' where \'Index\'=%s",cd.mac_addr.c_str(),row[7]);
		ksql_run_query(sql);
	}
	set_client_online(cd.ip,&cd);

	std::string cmd("iptables -A FORWARD -s ");

	cmd += cd.ip_addr;

	cmd += " -j ACCEPT";
	run_cmd(cmd);
}
static int TimedOut_InterVal=61000;

static void Wait_for_sql_change_or_timedout()
{
//	等待61秒看看
	Wait_For_event(nat_helper_event, TimedOut_InterVal);
}


static void port_map(MYSQL_ROW row, void * p)
{
	std::string cmd;
	cmd = "iptables -A PREROUTING -t nat -i eth+ -p ";
	cmd += row[1]; // protocol;
	cmd += " --dport ";
	cmd += row[2]; // wport
	cmd += " -j DNAT --to ";
	cmd += row[4]; //ip
	cmd += ":";
	cmd += row[3];//lport;
	run_cmd(cmd);
}

static void portmap_change(MYSQL_ROW row, void*p)
{
	int Is_ADD ;

	Is_ADD = atoi(row[1]);

	std::string cmd("iptables -");
	if(Is_ADD)
		cmd += "A ";
	else
		cmd += "D ";
	cmd += "PREROUTING -t nat -i eth+ -p ";

	cmd += row[2];
	cmd += "--dport ";
	cmd += row[3]; //wport
	cmd +=	" -j DNAT --to ";
	cmd += row[5];
	cmd += ":";
	cmd +=  row[4];//lport

	run_cmd(cmd);
	if(Is_Old_DB)
		sprintf((char*)p,"delete from t_portmap_tmp where nIndex <= %s",row[0]);
	else
		sprintf((char*)p,"delete from portmap_change where \'Index\' <= %s",row[0]);
}

static void room_change(MYSQL_ROW row,void*p)
{
	int Is_ADD;
	Is_ADD = atoi(row[1]);
	in_addr_t ip;
	ip = inet_addr(row[2]);
	if (Is_ADD) //添加
	{
		char* sql = new char[1024];
		if (Is_Old_DB)
		{
			sprintf(sql,
					"select CustomerName,CustomerIDType,CustomerIDNum,MachineIP,BuildNum,RoomFloor,RoomNum,nIndex from t_room_list where MachineIP =\'%s\'",
					row[2]);
		}
		else
		{
			sprintf(sql,
					"select Name,IDtype,ID,IP_ADDR,RoomBuild,RoomFloor,RoomNum,\'Index\' from room_list where IP_ADDR=\'%s\'",
					row[2]);
		}
		MYSQL_RES* res = ksql_query_and_use_result(sql);
		delete[] sql;
		if (res)
		{
			row = ksql_fetch_row(res);
			if(row)
			{
				loadroomlist(row,0);
			}
			ksql_free_result(res);
		}
	}else //删除
	{
		std::string cmd("iptables -D FORWARD -s ");

		cmd += row[2];
		cmd += " -j ACCEPT";

		run_cmd(cmd);

		set_client_offline(ip);
	}
	if(Is_Old_DB)
		sprintf((char*)p,"delete from t_customer_ln where nIndex <= %s",row[0]);
	else
		sprintf((char*)p,"delete from room_change where \'Index\' <= %s",row[0]);
}

static void *kroomthread(void*)
{
	char strSQL[1024];
	MYSQL_RES *	res;
	MYSQL_ROW	row;
	ksql_thread_init();

	for (;;)
	{
		std::cout << "waited" <<std::endl;

		Wait_for_sql_change_or_timedout();

		strSQL[0] = 0;
		//管理上下网

		ksql_query_and_use_result(room_change,
				Is_Old_DB ? "select nIndex,nType,MachineIP from t_customer_ln order by nIndex asc"
						    :"select * from room_change order by \'Index\' asc",
				strSQL);

		if (strSQL[0])
		{
			ksql_run_query(strSQL); //清除 room_change 或 t_customer_ln
			strSQL[0]=0;
		}

		if(Is_Old_DB)
		{
			ksql_query_and_use_result(portmap_change, "select nIndex,nType,proto,wanPort,lanPort,lanIpAddr from t_portmap_tmp order by nIndex asc",strSQL);
		}else
		{
			ksql_query_and_use_result(portmap_change, "select * from portmap_change order by \'Index\' asc",strSQL);
		}

		if (strSQL[0])
		{
			ksql_run_query(strSQL); //清除 portmap_change
			strSQL[0]=0;
		}

		if(Is_Old_DB) //旧数据库格式白名单是另外搞的，嘿嘿
		{
			// 白名单变动得查看看 t_legalip_tmp
			int type;
			res = ksql_query_and_use_result("select nIndex,nType,MachineIP from t_legalip_tmp order by nIndex asc");
			while ((row = ksql_fetch_row(res)))
			{
				in_addr_t  ip;
				type = atoi(row[1]);
				ip = inet_addr(row[2]);

				if(type) // 删除白名单
				{
					std::string cmd("iptables -D FORWARD -s ");
					cmd += row[2];
					cmd += " -j ACCEPT";
					run_cmd(cmd);

					set_client_offline(ip);

				}else // 添加白名单
				{
					char * strsql = new char[1024];
					sprintf(strsql,
							"select CustomerName,CustomerIDType,CustomerIDNum,MachineIP,BuildNum,RoomFloor,RoomNum,nIndex from t_room_list where MachineIP=\'%s\' ",
							row[2]);
					ksql_query_and_use_result(loadroomlist, strsql, 0);
					delete []strsql;
				}
				sprintf(strSQL,"delete from t_legalip_tmp where nIndex <= %s",row[0]);
			}
			ksql_free_result(res);
			if(strSQL[0])
			{
				ksql_run_query(strSQL);
				strSQL[0]=0;
			}
		}
	}
	ksql_thread_end();
}

extern "C" int __module_init(struct so_data*so)
{

	pthread_t p;

	nat_helper_event = so->nat_helper_event;

	std::cout << "\t初始化 iptables" << std::endl;
	std::string cmd;
	cmd = "iptables -F";
	run_cmd(cmd);

	cmd = "iptables -F -t nat";
	run_cmd(cmd);

	cmd = "iptables -P FORWARD DROP";
	run_cmd(cmd);

	cmd = "iptables -A FORWARD -i eth+ -j ACCEPT";
	run_cmd(cmd);

	cmd = "iptables -t nat -A POSTROUTING -o ";
	cmd += "eth+";
	cmd +=" -j MASQUERADE";
	run_cmd(cmd);
	std::cout << std::endl;

	//解析出参数来
	std::string pswd;

	pswd=GetToken(so->config_file,"password");
	TimedOut_InterVal = atoi(GetToken(so->config_file,"interval").c_str());
	if(TimedOut_InterVal==0)
		TimedOut_InterVal=61000;

	std::cout << "----------初始化数据库----------" << std::endl;

	if(InitRecordSQL(pswd))
			return -1;


	//进行判断是否为旧式数据库格式
	std::cout << "检查数据库格式为......";
	std::cout.flush();
	MYSQL_RES *res = ksql_query_and_use_result_quite("select * from room_list");
	if(!res)
	{
		ksql_free_result(res);
		Is_Old_DB = true;
//		sleep(1);

		std::cout << "老数据库格式\t兼容模式运行" <<std::endl;
		// 老旧的上网数据库。
	}else
	{
		Is_Old_DB = false;
		std::cout << "新数据库格式\tOK" <<std::endl;
	}

	std::cout << "\t加载允许上网客户" << std::endl;
	//从 room_list 加载允许上网的客户和其信息
	if(Is_Old_DB)
		ksql_query_and_use_result(loadroomlist,
				"select CustomerName,CustomerIDType,CustomerIDNum,MachineIP,BuildNum,RoomFloor,RoomNum,nIndex from t_room_list where bLogin=1",
				0);
	else
		ksql_query_and_use_result(loadroomlist,
				"select Name,IDtype,ID,IP_ADDR,RoomBuild,RoomFloor,RoomNum,\'Index\' from room_list where IsEmpty=0 and state>0 ",
				0);
	if (Is_Old_DB)
	{
		std::cout << "----------加载白名单----------" << std::endl;

		res = ksql_query_and_use_result("select * from t_legalip");

		MYSQL_ROW row;
		char * sqlstr = new char[1024];
		while ((row = ksql_fetch_row(res)))
		{
			struct Clients_DATA cd(row[1]);
			cd.CustomerName = row[2];
			sprintf(sqlstr,
					"select CustomerName,CustomerIDType,CustomerIDNum,MachineIP,BuildNum,RoomFloor,RoomNum,nIndex from t_room_list where MachineIP=\'%s\' ",
					row[1]);
			ksql_query_and_use_result(loadroomlist, sqlstr, 0);
		}
		delete[] sqlstr;
		ksql_free_result(res);
	}
	std::cout << "----------启动端口映射----------" << std::endl;

	if (Is_Old_DB)
	{
		ksql_query_and_use_result(port_map,"select \'nIndex\',proto,wanPort,lanPort,lanIpAddr  from t_portmap",0);
	}
	else
	{
		ksql_query_and_use_result(port_map,"select * from portmap",0);
	}
	pthread_attr_t p_attr;
	pthread_attr_init(&p_attr);

	pthread_attr_setdetachstate(&p_attr,PTHREAD_CREATE_DETACHED);//不需要考虑线程的退出状态吧？！
	pthread_attr_setscope(&p_attr,PTHREAD_SCOPE_SYSTEM);  //要使用内核线程

	pthread_create(&p,&p_attr,kroomthread,0);
	pthread_attr_destroy(&p_attr);
	return 0;
}

char module_name[]="客户网络管理模块";
