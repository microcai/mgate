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
#ifdef HAVE_GETTEXT
#include <locale.h>
#include <libintl.h>
#define _(x) gettext(x)
#define N_(x) (x)
#endif

#include <glib.h>

#include "libdreamtop.h"
#include "protocol_def.h"

static char socket_file[256] = "/tmp/monitor.socket";

static const gchar * config_file_name = "/var/www/application/config/config.ini";
static const gchar * module_dir = MODULES_PATH;

#ifdef ENABLE_HOTEL
static void port_map(KSQL_ROW row, void * p)
{
	GString * cmd = g_string_new("");

	g_string_printf(cmd,
			"iptables -A PREROUTING -t nat -i %s -p %s --dport %s -j DNAT --to %s:%s",
			hotel::str_ethID, // eth1
			row[1], // protocol
			row[2], // wport
			row[4], //ip
			row[3] // LAN port
	);

	run_cmd(cmd->str);
	g_string_free(cmd,1);
}

static void portmap_change(KSQL_ROW row, void*p)
{
//	select nIndex,protocol,wport,lport,IP_ADDR,WAN_IP,Is_ADD from portmap_change order by nIndex asc
	int Is_ADD;

	Is_ADD = atoi(row[6]);

	GString * cmd = g_string_new("");

	if(row[1] && row[2] && row[3] && row[4] && row[5])
	{
		g_string_printf(cmd,
			"iptables -%c PREROUTING -t nat -i %s -p %s --dport %s -j DNAT --to %s:%s",
			Is_ADD?'A':'D',
			hotel::str_ethID, // eth1
			row[1], // protocol
			row[2], // wport
			row[4], //ip
			row[3] // LAN port
			);
		run_cmd(cmd->str);
	}
	sprintf((char*) p, "delete from portmap_change where nIndex <= %d",atoi(row[0]?row[0]:"0"));
	g_string_free(cmd,1);
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
	GString  * sql = g_string_new("");
	KSQL_RES* res;
	KSQL_ROW roomer_row;
	u_char mac[6];

	int		ActionType;
	bool	need_rerun=false;

	ActionType = atoi(row[2]);
	in_addr_t ip;

	g_string_printf(sql,"select r.`CustomerName`,r.`IDtype`,r.`ID`,r.`IP_ADDR`,"
		"r.`MAC_ADDR`,l.`RoomBuild`,l.`RoomFloor`,l.`RoomNum`,r.`live_address`,"
		"r.`country`,r.`org`,l.`IsBind`,l.`MAC_ADDR`,r.`nIndex` FROM roomer_list r,room_list l "
		"where r.nIndex='%s' and l.nIndex=r.RoomId", row[1]);
	switch (ActionType)
	{
	case 0: //登录可以上网咯!!!
		res = ksql_query_and_use_result(sql->str);
		while ((roomer_row = ksql_fetch_row(res)))
		{
			char macadd[18];

			//那就从 ip 取得 mac 地址啊，快点啊
			if (roomer_row[3] )
			{
				if(assert_mac(roomer_row[4]))
				{
					convertMAC((char*)mac,roomer_row[4]);
					mac_set_allowed(mac,FALSE,0);
				}
				cd.ip = inet_addr(roomer_row[3]);
				strcpy(cd.ip_addr , roomer_row[3]);
				if (cd.ip != INADDR_ANY && cd.ip != INADDR_NONE)
				{
					if (GetMac(roomer_row[3], macadd, mac))
					{
						g_string_printf(sql,"update roomer_list set MAC_ADDR='%s',LoginTime=Time,OnlineTime=Time where nIndex='%s'",
								macadd,row[1]);
						ksql_run_query(sql->str);
						strcpy(cd.mac_addr ,macadd);
					}
					else continue;
				}else if (assert_mac(roomer_row[4]))
				{
					convertMAC((char*)mac, roomer_row[4]);
//					mac_set_allowed(mac,true);
					strcpy(cd.mac_addr , roomer_row[4]);
					memcpy(cd.MAC_ADDR, mac, 6);
				}else continue;
			}
			else if (assert_mac(roomer_row[4]))
			{
				convertMAC((char*)mac, roomer_row[4]);
//				mac_set_allowed(mac,true);
				strcpy(cd.mac_addr , roomer_row[4]);
				memcpy(cd.MAC_ADDR,mac,6);
			}
			//好了，现在是有 mac 了，哈哈哈哈哈

			strcpy(cd.CustomerID , roomer_row[2]);
			strcpy(cd.CustomerIDType , roomer_row[1]);
			strcpy(cd.CustomerName , roomer_row[0]);
			strcpy(cd.Build , roomer_row[5]);
			strcpy(cd.Floor , roomer_row[6]);
			strcpy(cd.RoomNum , roomer_row[7]);
			memcpy(cd.MAC_ADDR, mac, 6);
			cd.nIndex = atoi(roomer_row[13]);
			set_client_data(mac, &cd);
			if (!mac_is_alowed(mac))
				mac_set_allowed(mac, true,0);
			syslog(LOG_NOTICE,"登录 客户:%s\n", roomer_row[0]);
		}
		ksql_free_result(res);
		break;
	case 1: //换房

		break;
	case 2://退房
		res = ksql_query_and_use_result(sql->str);
		while((roomer_row=ksql_fetch_row(res)))
		{
			//寻找 mac 地址是否存在
			if( assert_mac(roomer_row[4]))
			{
				convertMAC((char*)mac,roomer_row[4]);
				mac_set_allowed(mac,0,0);
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

			Cd.ClientID = Cd.DateTime = GetDBTime_tm(GetCurrentTime());

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

			Cd.ClientID = Cd.DateTime = GetDBTime_tm(GetCurrentTime());

			Cd.Type = 1;

			RecordAccout(Cd);

			if(roomer_row[0])
				syslog(LOG_NOTICE, "退房 客户：%s\n", roomer_row[0]);

			g_string_printf(sql,"delete from roomer_list where nIndex='%s'",row[1]);
			ksql_run_query(sql->str);
		}
		ksql_free_result(res);
		break;
	case 3: //  登记上网
		res = ksql_query_and_use_result(sql->str);
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

			Cd.ClientID = Cd.DateTime = GetDBTime_tm(GetCurrentTime());

			if(roomer_row[8])
				utf8_gbk(Cd.Unit,sizeof(Cd.Unit),roomer_row[8],strlen(roomer_row[8]));
			if(roomer_row[9])
				utf8_gbk(Cd.Country,sizeof(Cd.Country),roomer_row[9],strlen(roomer_row[9]));
			if(roomer_row[10])
				utf8_gbk(Cd.Organ,sizeof(Cd.Organ),roomer_row[10],strlen(roomer_row[10]));

			if(assert_mac(roomer_row[4])) // 呵呵啊？ 居然是白名单啊
			{
				convertMAC((char*)mac,roomer_row[4]);
				// 开启却不
				mac_set_allowed(mac,true,0);
			}else 	// 客房是否绑定了 mac  地址？
			if (atoi(roomer_row[11]) && roomer_row[12] && strlen(roomer_row[12])==17 )
			{
				convertMAC((char*)mac,roomer_row[12]);

				//绑定的 mac 地址是否已经被人使用？
				if(get_client_data(mac,&cd))
				{
					//那好，使用新的 mac 地址
					for (int i = 0; i < 6; ++i)
					{
						Cd.ComputerMac[i * 2] = roomer_row[12][i * 3];
						Cd.ComputerMac[i * 2 + 1] = roomer_row[12][i * 3 + 1];
					}

					g_string_printf(sql,
							"update roomer_list set MAC_ADDR='%s',IP_ADDR=default where nIndex='%s'",
							roomer_row[12], row[1]);
					ksql_run_query(sql->str);
					g_string_printf(sql,
							"insert into room_change (RoomerId,ActionType) values ('%s',0)",row[1]);
					//现在，开启吧
					ksql_run_query(sql->str);
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
		g_string_printf(sql,"select r.`CustomerName`,r.`IDtype`,r.`ID`,r.`MAC_ADDR`"
			 " FROM roomer_list r where  r.nIndex='%s'", row[1]);
		res = ksql_query_and_use_result(sql->str);
		while ((roomer_row = ksql_fetch_row(res)))
		{
			if(assert_mac(roomer_row[3]))
			{
				convertMAC((char*)mac,roomer_row[3]);
				mac_set_allowed(mac,true,0);
				g_string_printf(sql,"delete from roomer_list where nIndex='%s'",row[1]);
				ksql_run_query(sql->str);

				syslog(LOG_NOTICE,"添加白名单, mac = %s\n",roomer_row[3]);
			}
		}
		ksql_free_result(res);
		break;
	case 5: //白名单删除
		g_string_printf(sql,"select r.`CustomerName`,r.`IDtype`,r.`ID`,r.`MAC_ADDR`"
			 " FROM roomer_list r where  r.nIndex='%s'", row[1]);

		res = ksql_query_and_use_result(sql->str);
		while ((roomer_row = ksql_fetch_row(res)))
		{
			if(assert_mac(roomer_row[3]))
			{
				convertMAC((char*)mac,roomer_row[3]);
				mac_set_allowed(mac,0,0);
				g_string_printf(sql,"delete from roomer_list where nIndex='%s'",row[1]);
				ksql_run_query(sql->str);

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
	g_string_free(sql,1);
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
//
//	MYSQL_RES * res;
//	MYSQL_ROW  row;
//	CString sql;
//
//	tm *pTm = GetCurrentTime();
//
//	if(pTm->tm_mon)
//		pTm->tm_mon --;
//	else
//	{
//		pTm->tm_year --;
//		pTm->tm_mon = 11;
//	}
//
//
//	sql.Format("select nIndex,RoomId from roomer_list where Time < '%d-%d-%d'",
//            pTm->tm_year+1900,pTm->tm_mon+1,pTm->tm_mday );
//
//	res = ksql_query_and_use_result(sql);
//	while ((row = ksql_fetch_row(res)))
//	{
//		sql.Format("insert into room_change (RoomerId,ActionType) values ('%s',2)",row[0]);
//		ksql_run_query(sql);
//
//		sql.Format("update room_list set RoomerCount=RoomerCount-1 where nIndex='%s' and RoomerCount>0",row[1]);
//		ksql_run_query(sql);
//	}
}

static void load_white(KSQL_ROW row, void*)
{
	u_char mac[6];
	if(assert_mac(row[0]))
	{
		convertMAC((char*)mac,row[0]);
		mac_set_allowed(mac,true,0);
	}
}

static void pre_load(KSQL_ROW row, void*)
{
//	"select nIndex,RoomBuild,RoomFloor,RoomNum,MAC_ADDR,IsBind,RoomerCount from room_list"
	KSQL_RES * res;
	KSQL_ROW 	mrow;

	int			roomer_count;
	Clients_DATA cd;

	GString * sql = g_string_new("");
	g_string_printf(sql,"select count(RoomId) from roomer_list where RoomId='%s'",row[0]);
	res = ksql_query_and_use_result(sql->str);
	mrow = ksql_fetch_row(res);
	roomer_count = atoi(mrow ? mrow[0] : "0");
	ksql_free_result(res);

	if(roomer_count) // 非空的房间哦，哈哈
	{
		g_string_printf(sql,"select CustomerName,IDtype,ID,MAC_ADDR,nIndex,IP_ADDR from roomer_list where RoomId='%s'",row[0]);

		res = ksql_query_and_use_result(sql->str);
		while ((mrow = ksql_fetch_row(res)))
		{
			if(mrow[0] && mrow[2] && mrow[3] && assert_mac(mrow[3])) // 只添加有 mac  地址的，说明是有登录过的，嘿嘿
			{
				strcpy(cd.mac_addr , mrow[3]);
				convertMAC((char*)cd.MAC_ADDR,mrow[3]);

				strcpy(cd.Build , row[1]);
				strcpy(cd.CustomerID , mrow[2]);
				strcpy(cd.CustomerIDType , mrow[1]);
				strcpy(cd.CustomerName , mrow[0]);
				strcpy(cd.Floor , row[2]);
				strcpy(cd.RoomNum , row[3]);
				cd.nIndex = atoi(mrow[4]);
				if (mrow[5])
				{
					cd.ip = inet_addr(mrow[5]);
					strcpy(cd.ip_addr, mrow[5]);
				}
				else
					cd.ip = 0;

				mac_set_allowed(cd.MAC_ADDR,1,cd.ip);
				set_client_data(cd.MAC_ADDR,&cd);
				if( mrow[0] && mrow[3])
					syslog(LOG_NOTICE,"预加载客户:%s ，mac='%s' \n",mrow[0],mrow[3]);
			}
		}
		ksql_free_result(res);
	}
	g_string_free(sql,1);
}
#endif

static void on_term(int )
{
//	ksql_close();
	exit(0);
}
#if ENABLE_HOTEL
static gboolean on_inotify(GIOChannel *source, GIOCondition condition, gpointer data)
{
	struct inotify_event ie;
	gsize read;
	if(G_IO_ERROR_NONE == g_io_channel_read(source,(gchar*)&ie,sizeof(ie),&read))
	{
		On_SQL_change();
	}
}
#endif

int main(int argc, char*argv[], char*env[])
{
	g_thread_init(NULL);
	g_set_application_name(PACKAGE_NAME);
	setlocale(LC_ALL,"");
	textdomain(GETTEXT_PACKAGE);
#ifdef DEBUG
	bindtextdomain(GETTEXT_PACKAGE,"/tmp/share/locale");
#endif

	gboolean createdb = FALSE;
	gchar *  domain_dir = NULL;
	gboolean flush_db = FALSE;


	GOptionEntry args[] =
	{
			{"flushdb",'f',0,G_OPTION_ARG_NONE,&flush_db,_("flash the db")},
			{"createdb",'c',0,G_OPTION_ARG_NONE,&createdb,_("Create database")},
			{"locale",'\0',0,G_OPTION_ARG_STRING,&domain_dir,_("set domain dir root"),N_("dir")},
			{0}
	};

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

	syslog(LOG_NOTICE, _("%s loaded at %s"), PACKAGE_NAME,	ctime(&t));

	GOptionContext * context;
	context = g_option_context_new("");
	g_option_context_add_main_entries(context,args,PACKAGE_NAME);
	g_option_context_parse(context,&argc,&argv,NULL);
	g_option_context_free(context);

	umask(0);

	close(open(socket_file, O_CREAT | O_CLOEXEC, 0666));

	conf_fd = open(config_file_name, O_RDONLY | O_CLOEXEC); //  不要被 fork继承啊

	GKeyFile * gkeyfile = g_key_file_new();

	if (g_key_file_load_from_file(gkeyfile, config_file_name,
			G_KEY_FILE_KEEP_TRANSLATIONS, NULL))
		syslog(LOG_WARNING, "Err opening config file");

	//解析出参数来

	GError * err = NULL;

	gchar * update_server, *update_trunk;

	MAX_PCAP_THREAD = g_key_file_get_integer(gkeyfile,"monitor","threads",&err);

	if(err)
	{
		MAX_PCAP_THREAD = 1;
		g_error_free(err);
		err = NULL;
	}

	gchar * dns = g_key_file_get_string(gkeyfile,"monitor","dns",&err);

	update_server = g_key_file_get_string(gkeyfile,"update","server",&err);

	update_trunk = g_key_file_get_string(gkeyfile,"update","trunk",&err);

	StartSQL();

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

		printf(_("all tables truncated!\n"));

		return 0;
	}

	WaitForSQLserver();

	strcpy(rif.ifr_name, hotel::str_ethID);
	int tmp = socket(AF_INET, SOCK_DGRAM, 0);
	if(ioctl(tmp, SIOCGIFADDR, &rif)==0)
		arg.ip = ((sockaddr_in*) (&(rif.ifr_addr)))->sin_addr.s_addr;
	else
	{
		syslog(LOG_CRIT,_("nic %s not enabled!"),hotel::str_ethID);
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
		syslog(LOG_CRIT, _("ERROR:can not open %s for capturing!\n"),
				hotel::str_ethID);
		closelog();
		return -1;
	}

	if (pcap_datalink(arg.pcap_handle) != DLT_EN10MB)
	{
		syslog(LOG_CRIT, _("ERROR:%s is not an ethernet adapter\n"),
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
		ptr = strtok(dns, ",");
		GString * cmd = g_string_new("");
		while (ptr)
		{
			g_string_printf(cmd,"iptables -t nat -A POSTROUTING -j MASQUERADE -o eth+ "
					"--dest %s", ptr);
			run_cmd(cmd->str);
			ptr = strtok(0,",");

		}

		g_string_free(cmd,1);

	}
#endif

	for (int i = 0; i < MAX_PCAP_THREAD; ++i)
	{
		g_thread_create((GThreadFunc)pcap_thread_func,&arg,FALSE,NULL);
	}


#ifdef ENABLE_HOTEL

	syslog(LOG_CRIT, "预加载客户端\n");

	//从 room_list 加载允许上网的客户和其信息

	ksql_query_and_use_result(pre_load,"select nIndex,RoomBuild,RoomFloor,RoomNum,MAC_ADDR,IsBind,RoomerCount from room_list");
	ksql_query_and_use_result(load_white,"select MAC_ADDR from whitelist LIMIT 0,1000");

	syslog(LOG_NOTICE, "----------启动端口映射----------\n");

	ksql_query_and_use_result(port_map, "select * from portmap", 0);

#endif

	int inotifyfd = inotify_init1(O_CLOEXEC);
	int socket_file_watch = inotify_add_watch(inotifyfd, socket_file,
			IN_CLOSE_WRITE | IN_DELETE_SELF);

#ifndef DEBUG
	//daemon(0,1);
#endif

	GMainLoop * loop;

#ifdef ENABLE_HOTEL

	GIOChannel * gio = g_io_channel_unix_new(inotifyfd);

	g_io_add_watch(gio,G_IO_IN,on_inotify,GINT_TO_POINTER(socket_file));
#endif

	loop = g_main_loop_new(NULL,FALSE);

	//update_server
	gchar * Check_update_param [2] = {update_server,update_trunk};
	g_timeout_add(5,Check_update,Check_update_param);
	g_main_loop_run(loop);
	return 0;
}
