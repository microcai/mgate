/*
 *      main.c
 *
 *      Copyright 2009 MicroCai <microcai@sina.com>
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
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


#include <iostream>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <sys/poll.h>
#include <sys/un.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <pthread.h>
#include <sys/mman.h>
#include <pcap.h>
#include <errno.h>

#include "libmicrocai.h"
#include "protocol_def.h"


void pcap_thread_func( struct pcap_thread_args * );

static char	socket_file[256] = "/tmp/monitor.socket";


#ifdef DEBUG
static char			config_file[256]="/module.conf";
static char			module_dir[256]="./build/modules";
#else
static char			config_file[256]="/etc/module.conf";
static char			module_dir[256]= MODULES_PATH;
#endif
static struct parameter_tags parameter[] =	{
		parameter_tags("--config", parameter_type::STRING, config_file,sizeof(config_file), "-f,--config\t模块参数配置文件，默认是 ./lib/module.conf"),
		parameter_tags("--module_dir", parameter_type::STRING, module_dir,sizeof(module_dir), "--module_dir\t\t模块位置"),
		parameter_tags()
};


static inline char hex2str(char str[])
{
	u_char r = (((str[0]&0x40)? (str[0]&0xbf)-0x37 : str[0]-'0' )<< 4 & 0xF0 )+((str[1]&0x40)? (str[1]&0xbf)-0x37 : str[1]-'0' );
	return *(char*)(&r);
}

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
		if(hotel::Is_Old_DB)
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
	if(hotel::Is_Old_DB)
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
		if (hotel::Is_Old_DB)
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
	if(hotel::Is_Old_DB)
		sprintf((char*)p,"delete from t_customer_ln where nIndex <= %s",row[0]);
	else
		sprintf((char*)p,"delete from room_change where \'Index\' <= %s",row[0]);
}

static void KroomMonitor()
{
	char strSQL[1024];
	MYSQL_ROW	row;

	int socketfd;
	int err = 1;

	//进行判断是否为旧式数据库格式
	std::cout << "检查数据库格式为......";
	std::cout.flush();
	MYSQL_RES *res = ksql_query_and_use_result_quite("select * from room_list");
	if(!res)
	{
		ksql_free_result(res);
		hotel::Is_Old_DB = true;
//		sleep(1);

		std::cout << "老数据库格式\t兼容模式运行" <<std::endl;
		// 老旧的上网数据库。
	}else
	{
		hotel::Is_Old_DB = false;
		std::cout << "新数据库格式\tOK" <<std::endl;
	}

	std::cout << "\t加载允许上网客户" << std::endl;
	//从 room_list 加载允许上网的客户和其信息
	if(hotel::Is_Old_DB)
		ksql_query_and_use_result(loadroomlist,
				"select CustomerName,CustomerIDType,CustomerIDNum,MachineIP,BuildNum,RoomFloor,RoomNum,nIndex from t_room_list where bLogin=1",
				0);
	else
		ksql_query_and_use_result(loadroomlist,
				"select Name,IDtype,ID,IP_ADDR,RoomBuild,RoomFloor,RoomNum,\'Index\' from room_list where IsEmpty=0 and state>0 ",
				0);
	if (hotel::Is_Old_DB)
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

	if (hotel::Is_Old_DB)
	{
		ksql_query_and_use_result(port_map,"select \'nIndex\',proto,wanPort,lanPort,lanIpAddr  from t_portmap",0);
	}
	else
	{
		ksql_query_and_use_result(port_map, "select * from portmap", 0);
	}

	do
	{
		/*
		 * 这里使用SOCK_DGRAM来保留消息边界,同时避免建立链接的步骤，我不想
		 * 调式有链接的代码了。讨厌。
		 */
		socketfd = socket(AF_UNIX, SOCK_DGRAM, 0);
		fcntl(socketfd, F_SETFD, O_CLOEXEC); // 不要被 fork 继承
		if (socketfd <= 0)
			break;
		sockaddr_un sock =
		{ 0 };
		sock.sun_family = AF_UNIX;
		strncpy(sock.sun_path, socket_file, 106);
		unlink(socket_file);
		setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &err, sizeof(err));
		if (bind(socketfd, (struct sockaddr*) &sock, sizeof(sock)) < 0)
		{
			std::cerr << strerror(errno);
			break;
		}
		err = 0;

	} while (0);

	if (err)
		return;

	for (;;)
	{
		log_printf(L_DEBUG_OUTPUT,"%s:%s waited timeout\n",__FILE__,__func__);

//		Wait_for_sql_change_or_timedout
		//******************************************************
		pollfd pfd;
		pfd.fd = socketfd;
		pfd.events = POLLIN;
		poll(&pfd, 1, TimedOut_InterVal);

		sockaddr_un saddr;
		socklen_t saddr_len = sizeof(sockaddr_un);

		recvfrom( socketfd, strSQL,256,MSG_DONTWAIT,(sockaddr*)&saddr,&saddr_len);

		//******************************************************

		strSQL[0] = 0;
		//管理上下网

		ksql_query_and_use_result(room_change,
				hotel::Is_Old_DB ? "select nIndex,nType,MachineIP from t_customer_ln order by nIndex asc"
						    :"select * from room_change order by \'Index\' asc",
				strSQL);

		if (strSQL[0])
		{
			ksql_run_query(strSQL); //清除 room_change 或 t_customer_ln
			strSQL[0]=0;
		}

		if(hotel::Is_Old_DB)
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

		if(hotel::Is_Old_DB) //旧数据库格式白名单是另外搞的，嘿嘿
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
							"select CustomerName,CustomerIDType,CustomerIDNum,MachineIP,BuildNum,RoomFloor,RoomNum,nIndex from t_room_list where MachineIP=\'%s\'",
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
    close(socketfd);
    unlink(socket_file);
}

static void Init_IPtables()
{
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
}

int main(int argc, char*argv[], char*env[])
{
#ifdef DEBUG
	{
		auto_str p(new char[800]);
		getcwd(p,800);
		log_printf(L_DEBUG_OUTPUT,"current dir is :%s\n",(char*)p);
	}
#endif
	pthread_t pcap_tcp, pcap_udp;

	struct so_data pa;
	int	   conf_fd;
	struct stat st;

	struct pcap_thread_args arg1={0}, arg2={0};
	struct ifreq rif={{{0}}};
    time_t t;
    t = ::time(0);
    log_printf( L_DEBUG_OUTPUT_MORE,"%s loaded at %s\n",PACKAGE_NAME,ctime(&t));
	ParseParameters(&argc,&argv,parameter);


	strcpy(rif.ifr_name,hotel::str_ethID);

	int tmp = socket(AF_INET, SOCK_DGRAM, 0);
	ioctl(tmp, SIOCGIFADDR, &rif);
	arg2.ip = arg1.ip = ((sockaddr_in*) (&(rif.ifr_addr)))->sin_addr.s_addr;
	ioctl(tmp, SIOCGIFNETMASK, &rif);
	arg2.mask = arg1.mask = ((sockaddr_in*) (&(rif.ifr_addr)))->sin_addr.s_addr;
	close(tmp);

	setuid(0);

	conf_fd = open(config_file, O_RDONLY|O_CLOEXEC); //  不要被 fork继承啊

	if (conf_fd > 0)
	{
		fstat(conf_fd, &st);
		pa.config_file = (char*) mmap(0, st.st_size ? st.st_size : 1, PROT_READ, MAP_PRIVATE, conf_fd, 0);
		close(conf_fd);
	}
	else
	{
		log_printf(L_ERROR, "Err openning config file\n");
		pa.config_file = NULL;
	}


	// kroom 管理的初始化
	Init_IPtables();

	//解析出参数来
	std::string pswd,user,host,database;

	pswd=GetToken(pa.config_file,"password");
	user=GetToken(pa.config_file,"username","root");
	host=GetToken(pa.config_file,"sql_host","localhost");
	database=GetToken(pa.config_file,"database","hotel");
	TimedOut_InterVal = atoi(GetToken(pa.config_file,"interval","61000").c_str());

	log_printf(L_DEBUG_OUTPUT,"user=%s,pawd=%s,host=%s,db=%s,timeout=%d\n",user.c_str(),pswd.c_str(),host.c_str(),database.c_str(),TimedOut_InterVal);

	std::cout << "----------初始化数据库----------" << std::endl;

	if(InitRecordSQL(pswd,user,database,host))
			return -1;

	if (enum_and_load_modules(module_dir, &pa))
		return (0);

	if (conf_fd > 0)
		munmap(pa.config_file, st.st_size ? st.st_size : 1);

	strncpy(arg1.eth, hotel::str_ethID, 8);
	strncpy(arg2.eth, hotel::str_ethID, 8);

	strncpy(arg1.bpf_filter_string, "tcp", 200);
	strncpy(arg2.bpf_filter_string, "udp", 200);

	pthread_attr_t p_attr;
	pthread_attr_init(&p_attr);

	pthread_attr_setdetachstate(&p_attr,PTHREAD_CREATE_DETACHED);//不需要考虑线程的退出状态吧？！
	pthread_attr_setscope(&p_attr,PTHREAD_SCOPE_SYSTEM);  //要使用内核线程

	pthread_create(&pcap_tcp, &p_attr, (void *(*)(void *)) pcap_thread_func, &arg1);
	pthread_create(&pcap_udp, &p_attr, (void *(*)(void *)) pcap_thread_func, &arg2);

	pthread_attr_destroy(&p_attr);

	KroomMonitor();

#ifdef DEBUG
	sleep(-1);
#endif
	return 0;
}

