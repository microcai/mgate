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
#include <sys/signal.h>
#include <sys/inotify.h>
#include <sys/poll.h>
#include <sys/syslog.h>
#include <dirent.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <pthread.h>

#include <sys/mman.h>
#include <pcap.h>
#include <errno.h>

#include "libdreamtop.h"
#include "protocol_def.h"

static char socket_file[256] = "/tmp/monitor.socket";

static char config_file_name[256] = "/var/www/application/config/config.ini";
static char module_dir[256] = MODULES_PATH;
static int flush_db = 0;

static struct parameter_tags parameter[] =
{
		parameter_tags("--config", parameter_type::STRING, config_file_name,
				sizeof(config_file_name),
				"-f,--config\t模块参数配置文件，默认是 ./lib/module.conf"), parameter_tags(
				"--module_dir", parameter_type::STRING, module_dir,
				sizeof(module_dir), "--module_dir\t\t模块位置"), parameter_tags(
				"--flushdb", parameter_type::BOOL_short, (char*) &flush_db,
				sizeof(flush_db), "--flush_db\t\t清空客房数据"), parameter_tags()
};
#ifdef ENABLE_HOTEL
static void port_map(KSQL_ROW row, void * p)
{
	CString cmd;

	cmd.Format(
			"iptables -A PREROUTING -t nat -i %s -p %s --dport %s -j DNAT --to %s:%s",
			hotel::str_ethID, // eth1
			row[1], // protocol
			row[2], // wport
			row[4], //ip
			row[3] // LAN port
	);

	run_cmd(cmd);
}

static void portmap_change(KSQL_ROW row, void*p)
{
//	select nIndex,protocol,wport,lport,IP_ADDR,WAN_IP,Is_ADD from portmap_change order by nIndex asc
	int Is_ADD;

	Is_ADD = atoi(row[6]);

	CString cmd;

	if(row[1] && row[2] && row[3] && row[4] && row[5])
	{
		cmd.Format(
			"iptables -%c PREROUTING -t nat -i %s -p %s --dport %s -j DNAT --to %s:%s",
			Is_ADD?'A':'D',
			hotel::str_ethID, // eth1
			row[1], // protocol
			row[2], // wport
			row[4], //ip
			row[3] // LAN port
			);
		run_cmd(cmd);
	}
	sprintf((char*) p, "delete from portmap_change where nIndex <= %d",atoi(row[0]?row[0]:"0"));
}



static bool assert_mac(const char * strmac)
{
	char mac[6];
	char mmac[6];
	bzero(mmac,6);
	if(strmac && strlen(strmac) == 17)
	{
		convertMAC(mac,strmac);
		return memcpy(mac,mmac,6);
	}
	return false;
}

static void room_change(KSQL_ROW row, void*p)
{

	//select nIndex,RoomerId,ActionType from room_change order by nIndex asc"

	CustomerData Cd;
	Clients_DATA cd;
	CString sql;
	KSQL_RES* res;
	KSQL_ROW roomer_row;
	u_char mac[6];

	int		ActionType;
	bool	need_rerun=false;

	ActionType = atoi(row[2]);
	in_addr_t ip;

	sql.Format("select r.`CustomerName`,r.`IDtype`,r.`ID`,r.`IP_ADDR`,"
		"r.`MAC_ADDR`,l.`RoomBuild`,l.`RoomFloor`,l.`RoomNum`,r.`live_address`,"
		"r.`country`,r.`org`,l.`IsBind`,l.`MAC_ADDR`,r.`nIndex` FROM roomer_list r,room_list l "
		"where r.nIndex='%s' and l.nIndex=r.RoomId", row[1]);
	switch (ActionType)
	{
	case 0: //登录可以上网咯!!!
		res = ksql_query_and_use_result(sql);
		while ((roomer_row = ksql_fetch_row(res)))
		{
			char macadd[18];

			//那就从 ip 取得 mac 地址啊，快点啊
			if (roomer_row[3] )
			{
				if(assert_mac(roomer_row[4]))
				{
					convertMAC(mac,roomer_row[4]);
					mac_set_allowed(mac);
				}
				cd.ip = inet_addr(roomer_row[3]);
				cd.ip_addr = roomer_row[3];
				if (cd.ip != INADDR_ANY && cd.ip != INADDR_NONE)
				{
					if (GetMac(roomer_row[3], macadd, mac))
					{
						sql.Format("update roomer_list set MAC_ADDR='%s',LoginTime=Time,OnlineTime=Time where nIndex='%s'",
								macadd,row[1]);
						ksql_run_query(sql);
						cd.mac_addr = macadd;
					}
					else continue;
				}else if (assert_mac(roomer_row[4]))
				{
					convertMAC(mac, roomer_row[4]);
//					mac_set_allowed(mac,true);
					cd.mac_addr = roomer_row[4];
					memcpy(cd.MAC_ADDR, mac, 6);
				}else continue;
			}
			else if (assert_mac(roomer_row[4]))
			{
				convertMAC(mac, roomer_row[4]);
//				mac_set_allowed(mac,true);
				cd.mac_addr = roomer_row[4];
				memcpy(cd.MAC_ADDR,mac,6);
			}
			//好了，现在是有 mac 了，哈哈哈哈哈
			cd.CustomerID = roomer_row[2];
			cd.CustomerIDType = roomer_row[1];
			cd.CustomerName = roomer_row[0];
			cd.Build = roomer_row[5];
			cd.Floor = roomer_row[6];
			cd.RoomNum = roomer_row[7];
			memcpy(cd.MAC_ADDR, mac, 6);
			cd.nIndex = atoi(roomer_row[13]);
			set_client_data(mac, &cd);
			if (!mac_is_alowed(mac))
				mac_set_allowed(mac, true);
			syslog(LOG_NOTICE,"登录 客户:%s\n", roomer_row[0]);
		}
		ksql_free_result(res);
		break;
	case 1: //换房

		break;
	case 2://退房
		res = ksql_query_and_use_result(sql);
		while((roomer_row=ksql_fetch_row(res)))
		{
			//寻找 mac 地址是否存在
			if( assert_mac(roomer_row[4]))
			{
				convertMAC(mac,roomer_row[4]);
				mac_set_allowed(mac);
				for (int i = 0; i < 6; ++i)
				{
					Cd.ComputerMac[i * 2] = roomer_row[4][i * 3];
					Cd.ComputerMac[i * 2 + 1] = roomer_row[4][i * 3 + 1];
				}
			} //好了，已经是不可以在线了哦

			if(roomer_row[1])
				strncpy(Cd.CertType, roomer_row[1], sizeof(Cd.CertType));
			if(roomer_row[2])
				strncpy(Cd.CertNo, roomer_row[2], sizeof(Cd.CertNo));
			if(roomer_row[0])
				utf8_gbk(Cd.ClientName, sizeof(Cd.ClientName), roomer_row[0], strlen(roomer_row[0]));
			if(roomer_row[5] && roomer_row[6] && roomer_row[7])
				snprintf(Cd.ComputerName, sizeof(Cd.ComputerName), "%c%c%02d",
						roomer_row[5][0], roomer_row[6][0], atoi(roomer_row[7]));

			Cd.ClientID = Cd.DateTime = GetDBTime(GetCurrentTime());

			if(roomer_row[8])
				utf8_gbk(Cd.Unit,sizeof(Cd.Unit),roomer_row[8],strlen(roomer_row[8]));
			if(roomer_row[9])
				utf8_gbk(Cd.Country,sizeof(Cd.Country),roomer_row[9],strlen(roomer_row[9]));
			if(roomer_row[10])
				utf8_gbk(Cd.Organ,sizeof(Cd.Organ),roomer_row[10],strlen(roomer_row[10]));

			if (roomer_row[3] && strlen(roomer_row[3]))
			{
				ip = inet_addr(roomer_row[3]);
				snprintf(Cd.ComputerIp, sizeof(Cd.ComputerIp),
						"%03d.%03d.%03d.%03d", ((u_char*) &ip)[0],
						((u_char*) &ip)[1], ((u_char*) &ip)[2],
						((u_char*) &ip)[3]);
			}

			if(roomer_row[5] && roomer_row[6] && roomer_row[7])
				snprintf(Cd.ComputerName, sizeof(Cd.ComputerName), "%c%c%02d",
						roomer_row[5][0], roomer_row[6][0], atoi(roomer_row[7]));

			Cd.ClientID = Cd.DateTime = GetDBTime(GetCurrentTime());

			Cd.Type = 1;

			RecordAccout(Cd);

			if(roomer_row[0])
				syslog(LOG_NOTICE, "退房 客户：%s\n", roomer_row[0]);

			sql.Format("delete from roomer_list where nIndex='%s'",row[1]);
			ksql_run_query(sql);
		}
		ksql_free_result(res);
		break;
	case 3: //  登记上网
		res = ksql_query_and_use_result(sql);
		while ((roomer_row = ksql_fetch_row(res)))
		{
			if(roomer_row[1])
				strncpy(Cd.CertType, roomer_row[1], sizeof(Cd.CertType));
			if(roomer_row[2])
				strncpy(Cd.CertNo, roomer_row[2], sizeof(Cd.CertNo));
			if(roomer_row[0])
				utf8_gbk(Cd.ClientName, sizeof(Cd.ClientName), roomer_row[0], strlen(roomer_row[0]));
			if(roomer_row[5] && roomer_row[6] && roomer_row[7])
				snprintf(Cd.ComputerName, sizeof(Cd.ComputerName), "%c%c%02d",
						roomer_row[5][0], roomer_row[6][0], atoi(roomer_row[7]));

			Cd.ClientID = Cd.DateTime = GetDBTime(GetCurrentTime());

			if(roomer_row[8])
				utf8_gbk(Cd.Unit,sizeof(Cd.Unit),roomer_row[8],strlen(roomer_row[8]));
			if(roomer_row[9])
				utf8_gbk(Cd.Country,sizeof(Cd.Country),roomer_row[9],strlen(roomer_row[9]));
			if(roomer_row[10])
				utf8_gbk(Cd.Organ,sizeof(Cd.Organ),roomer_row[10],strlen(roomer_row[10]));

			if(assert_mac(roomer_row[4])) // 呵呵啊？ 居然是白名单啊
			{
				convertMAC(mac,roomer_row[4]);
				// 开启却不
				mac_set_allowed(mac,true);
			}else 	// 客房是否绑定了 mac  地址？
			if (atoi(roomer_row[11]) && roomer_row[12] && strlen(roomer_row[12])==17 )
			{
				convertMAC(mac,roomer_row[12]);

				//绑定的 mac 地址是否已经被人使用？
				if(get_client_data(mac,&cd))
				{
					//那好，使用新的 mac 地址
					for (int i = 0; i < 6; ++i)
					{
						Cd.ComputerMac[i * 2] = roomer_row[12][i * 3];
						Cd.ComputerMac[i * 2 + 1] = roomer_row[12][i * 3 + 1];
					}

					sql.Format(
							"update roomer_list set MAC_ADDR='%s',IP_ADDR=default where nIndex='%s'",
							roomer_row[12], row[1]);
					ksql_run_query(sql);
					sql.Format(
							"insert into room_change (RoomerId,ActionType) values ('%s',0)",row[1]);
					//现在，开启吧
					ksql_run_query(sql);
					need_rerun = true;
				}
			}
			RecordAccout(Cd);
			if(roomer_row[0] && roomer_row[2])
				syslog(LOG_NOTICE, "登记客户:%s , ID = %s\n", roomer_row[0],roomer_row[2]);
		}
		ksql_free_result(res);
		break;
	case 4: ///白名单添加
		sql.Format("select r.`CustomerName`,r.`IDtype`,r.`ID`,r.`MAC_ADDR`"
			 " FROM roomer_list r where  r.nIndex='%s'", row[1]);
		res = ksql_query_and_use_result(sql);
		while ((roomer_row = ksql_fetch_row(res)))
		{
			if(assert_mac(roomer_row[3]))
			{
				convertMAC(mac,roomer_row[3]);
				mac_set_allowed(mac,true);
				sql.Format("delete from roomer_list where nIndex='%s'",row[1]);
				ksql_run_query(sql);

				syslog(LOG_NOTICE,"添加白名单, mac = %s\n",roomer_row[3]);
			}
		}
		ksql_free_result(res);
		break;
	case 5: //白名单删除
		sql.Format("select r.`CustomerName`,r.`IDtype`,r.`ID`,r.`MAC_ADDR`"
			 " FROM roomer_list r where  r.nIndex='%s'", row[1]);

		res = ksql_query_and_use_result(sql);
		while ((roomer_row = ksql_fetch_row(res)))
		{
			if(assert_mac(roomer_row[3]))
			{
				convertMAC(mac,roomer_row[3]);
				mac_set_allowed(mac);
				sql.Format("delete from roomer_list where nIndex='%s'",row[1]);
				ksql_run_query(sql);

				syslog(LOG_NOTICE,"删除白名单, mac = %s\n",roomer_row[3]);
			}
		}
		ksql_free_result(res);
	}

	sprintf((char*) p, "delete from room_change where nIndex <= %s", row[0]);
	if (need_rerun)
	{
		((char*) p)[0] = -1;
	}
}

static void On_SQL_change()
{
	char strSQL[1024];

	strSQL[0] = 0;
	//管理上下网

	do
	{
		ksql_query_and_use_result(room_change,
				"select nIndex,RoomerId,ActionType from room_change order by nIndex asc", strSQL);

		if (strSQL[0] !=0 )
		{
			int old = strSQL[0];
			strSQL[0] = 'd';
			ksql_run_query(strSQL); //清除 room_change
			strSQL[0] = old;
		}
	} while (strSQL[0] == -1);

	strSQL[0] = 0;

	ksql_query_and_use_result(portmap_change,
			"select nIndex,protocol,wport,lport,IP_ADDR,WAN_IP,Is_ADD from portmap_change order by nIndex asc", strSQL);

	if (strSQL[0])
	{
		ksql_run_query(strSQL); //清除 portmap_change
		strSQL[0] = 0;
	}
	//检查是否有人住的太太太久了，删掉他！

	MYSQL_RES * res;
	MYSQL_ROW  row;
	CString sql;

	tm *pTm = GetCurrentTime();

	if(pTm->tm_mon)
		pTm->tm_mon --;
	else
	{
		pTm->tm_year --;
		pTm->tm_mon = 11;
	}


	sql.Format("select nIndex,RoomId from roomer_list where Time < '%d-%d-%d'",
            pTm->tm_year+1900,pTm->tm_mon+1,pTm->tm_mday );

	res = ksql_query_and_use_result(sql);
	while ((row = ksql_fetch_row(res)))
	{
		sql.Format("insert into room_change (RoomerId,ActionType) values ('%s',2)",row[0]);
		ksql_run_query(sql);

		sql.Format("update room_list set RoomerCount=RoomerCount-1 where nIndex='%s' and RoomerCount>0",row[1]);
		ksql_run_query(sql);
	}
}

static void load_white(KSQL_ROW row, void*)
{
	u_char mac[6];
	if(assert_mac(row[0]))
	{
		convertMAC(mac,row[0]);
		mac_set_allowed(mac,true);
	}
}

static void pre_load(KSQL_ROW row, void*)
{
//	"select nIndex,RoomBuild,RoomFloor,RoomNum,MAC_ADDR,IsBind,RoomerCount from room_list"
	KSQL_RES * res;
	KSQL_ROW 	mrow;

	int			roomer_count;
	Clients_DATA cd;

	CString sql;
	sql.Format("select count(RoomId) from roomer_list where RoomId='%s'",row[0]);
	res = ksql_query_and_use_result(sql);
	mrow = ksql_fetch_row(res);
	roomer_count = atoi(mrow ? mrow[0] : "0");
	ksql_free_result(res);

	if(roomer_count) // 非空的房间哦，哈哈
	{
		sql.Format("select CustomerName,IDtype,ID,MAC_ADDR,nIndex,IP_ADDR from roomer_list where RoomId='%s'",row[0]);

		res = ksql_query_and_use_result(sql);
		while ((mrow = ksql_fetch_row(res)))
		{
			if(mrow[0] && mrow[2] && mrow[3] && assert_mac(mrow[3])) // 只添加有 mac  地址的，说明是有登录过的，嘿嘿
			{
				cd.mac_addr = mrow[3];
				convertMAC(cd.MAC_ADDR,mrow[3]);

				cd.Build = row[1];
				cd.CustomerID = mrow[2];
				cd.CustomerIDType = mrow[1];
				cd.CustomerName = mrow[0];
				cd.Floor = row[2];
				cd.RoomNum = row[3];
				cd.nIndex = atoi(mrow[4]);
				if(mrow[5])
				{
					cd.ip = inet_addr(mrow[5]);
					cd.ip_addr = mrow[5];
				}
				else cd.ip=0;

				mac_set_allowed(cd.MAC_ADDR,true,cd.ip);
				set_client_data(cd.MAC_ADDR,&cd);
				if( mrow[0] && mrow[3])
					syslog(LOG_NOTICE,"预加载客户:%s ，mac='%s' \n",mrow[0],mrow[3]);
			}
		}
		ksql_free_result(res);
	}
}
#endif

static void on_term(int )
{
//	ksql_close();
	exit(0);
}

int main(int argc, char*argv[], char*env[])
{
	pthread_attr_t p_attr;
	pthread_t pcap_tcp;
	time_t t;

	char errbuf[PCAP_ERRBUF_SIZE];
	struct bpf_program bpf_filter =
	{ 0 };

	int conf_fd;
	int MAX_PCAP_THREAD;
	struct stat st;

	struct pcap_thread_args arg;

	struct ifreq rif ;

	bzero(&rif,sizeof(ifreq));

	time(&t);

#ifdef DEBUG
	openlog(PACKAGE_TARNAME,LOG_PERROR|LOG_PID,LOG_USER);
#else
	openlog(PACKAGE_TARNAME,LOG_PID,LOG_USER);
#endif
#if 00
	//调用 检查自动升级。
	while(1){Check_update("localhost","");sleep(1);}
#endif
	//exit (0);

	syslog(LOG_NOTICE, "%s loaded at %s", PACKAGE_NAME,	ctime(&t));

	ParseParameters(&argc, &argv, parameter);

	umask(0);

	close(open(socket_file, O_CREAT | O_CLOEXEC, 0666));

	conf_fd = open(config_file_name, O_RDONLY | O_CLOEXEC); //  不要被 fork继承啊

	if (conf_fd > 0)
	{
		char *config_file;

		fstat(conf_fd, &st);
		config_file = (char*) mmap(0, st.st_size ? st.st_size : 1, PROT_READ,
				MAP_PRIVATE, conf_fd, 0);
		close(conf_fd);
		prase_config(config_file,st.st_size ? st.st_size : 1);
		munmap(config_file, st.st_size ? st.st_size : 1);
	}
	else
		syslog(LOG_WARNING, "Err opening config file");

	//解析出参数来
	std::string dnss,threads,update_server,update_trunk;

	threads = GetToken("threads","1");
	dnss = GetToken("dns", "");
	update_server = GetToken("update.server","");
	update_trunk = GetToken("update.trunk","monitor");
	update_server = GetToken(config_file,"update.server","");
	update_trunk = GetToken(config_file,"update.trunk","monitor");
	MAX_PCAP_THREAD = atoi(threads.c_str());

	StartSQL();

	signal(15,on_term);
	signal(2,on_term);

	signal(15,on_term);
	signal(2,on_term);


	if(flush_db)
	{

		ksql_run_query("truncate roomer_list");
		ksql_run_query("truncate room_list");
		ksql_run_query("truncate room_change");
		ksql_run_query("truncate portmap");
		ksql_run_query("truncate portmap_change");
		ksql_run_query("truncate t_customerlog");
		ksql_run_query("truncate t_floor");
		ksql_run_query("truncate t_netlog");
		ksql_run_query("truncate whitelist");

		printf("all tables truncated!\n");

		return 0;
	}

	WaitForSQLserver();

	strcpy(rif.ifr_name, hotel::str_ethID);
	int tmp = socket(AF_INET, SOCK_DGRAM, 0);
	if(ioctl(tmp, SIOCGIFADDR, &rif)==0)
		arg.ip = ((sockaddr_in*) (&(rif.ifr_addr)))->sin_addr.s_addr;
	else
	{
		syslog(LOG_CRIT,"nic %s not enabled!",hotel::str_ethID);
		sleep(20);
		return 1;
	}

	ioctl(tmp, SIOCGIFNETMASK, &rif);
	arg.mask = ((sockaddr_in*) (&(rif.ifr_addr)))->sin_addr.s_addr;

	ioctl(tmp, SIOCGIFHWADDR, &rif);
	memcpy(arg.mac_addr,rif.ifr_hwaddr.sa_data,6);

	close(tmp);

	arg.pcap_handle = pcap_open_live(hotel::str_ethID, 65536, 0, 0,
			errbuf);
	if(!arg.pcap_handle)
	{
		syslog(LOG_CRIT, "ERROR:can not open %s for capturing!\n",
				hotel::str_ethID);
		closelog();
		return -1;
	}

	if (pcap_datalink(arg.pcap_handle) != DLT_EN10MB)
	{
		syslog(LOG_CRIT, "ERROR:%s is not an ethernet adapter\n",
				hotel::str_ethID);
		return -1;
	}

	if (enum_and_load_modules(module_dir))
		return (0);


	pcap_compile(arg.pcap_handle, &bpf_filter, "tcp or udp", 1, arg.mask);

	pcap_setfilter(arg.pcap_handle, &bpf_filter);

	pcap_freecode(&bpf_filter);

	#ifdef ENABLE_HOTEL

	run_cmd("iptables -F -t nat");
	{
		char * ptr;
		ptr = strtok((char *) dnss.c_str(), ",");
		while (ptr)
		{
			CString cmd;
			cmd.Format("iptables -t nat -A POSTROUTING -j MASQUERADE -o eth+ "
					"--dest %s", ptr);
			run_cmd(cmd);
			ptr = strtok(0,",");
		}

	}
	#endif

	pthread_attr_init(&p_attr);
	pthread_attr_setdetachstate(&p_attr, PTHREAD_CREATE_DETACHED);//不需要考虑线程的退出状态吧？！

	for (int i = 0; i < MAX_PCAP_THREAD; ++i)
	{
		pthread_create(&pcap_tcp, &p_attr,
				(void *(*)(void *)) pcap_thread_func, &arg);
	}

	pthread_attr_destroy(&p_attr);

#ifdef ENABLE_HOTEL

	syslog(LOG_CRIT, "预加载客户端\n");

	//从 room_list 加载允许上网的客户和其信息

	ksql_query_and_use_result(pre_load,"select nIndex,RoomBuild,RoomFloor,RoomNum,MAC_ADDR,IsBind,RoomerCount from room_list");
	ksql_query_and_use_result(load_white,"select MAC_ADDR from whitelist LIMIT 0,1000");

	syslog(LOG_NOTICE, "----------启动端口映射----------\n");

	ksql_query_and_use_result(port_map, "select * from portmap", 0);

	RE_INOTIFY:

	int inotifyfd = inotify_init1(O_CLOEXEC);
	int socket_file_watch = inotify_add_watch(inotifyfd, socket_file,
			IN_CLOSE_WRITE | IN_DELETE_SELF);
//	int module_dir_watch = inotify_add_watch(inotifyfd, module_dir,
//			IN_CLOSE_WRITE | IN_DELETE);

#ifndef DEBUG
	//daemon(0,1);
#endif

	//signal(SIGSEGV, on_sigsegv);

	for (;;)
	{
		pollfd pfd[1];
		pfd[0].fd = inotifyfd;
		pfd[0].events = POLLIN|POLLERR;

		inotify_event *inotifyevent = (inotify_event*) errbuf;

		int timedout = 1;
		switch (poll(pfd, 1, timedout *1000))
		{
		case 0:
			On_SQL_change();
			//调用 检查自动升级。
			switch(Check_update(update_server.c_str(),update_trunk.c_str()))
			{
				case -1:
				timedout=5;
				break;
				case 0:
				timedout=1;
			}

			break;
		case 1:
			if (read(inotifyfd, inotifyevent, sizeof(errbuf)) > 0)
			{
				if (inotifyevent->wd == socket_file_watch)
					On_SQL_change();
			}
			else
			{
				if (errno != EINTR || errno != EAGAIN)
				{
					close(inotifyfd);
					goto RE_INOTIFY;
				}
			}
			break;
		case -1:
			if (errno == EINTR)
				continue;
			close(inotifyfd);
			goto RE_INOTIFY;
		}
	}
	return 0;
#else
	if (ksql_is_server_gone())
	{
		{
		ksql_close();
		while (InitRecordSQL(pswd, user, database, host))
			sleep(2);
			sleep(5);
			break;
		case 0:
			sleep(1);
			break;
		}
	}
	sleep(5000);
#endif

}

