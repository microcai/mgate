/*
 * kServer.cpp
 *
 *  Created on: 2009-5-11
 *      Author: cai
 */

/*
 * 这个是个绝对可选的模块。 在于为monitor提供一个网页管理的替代方案。
 *
 */
#include <iterator>
#include <iostream>
#include <map>
#include <deque>
#include <stdio.h>
#include <stdlib.h>
#include <sys/poll.h>
#include <netinet/in.h>
#include <net/ethernet.h>
//We need md5
#include <fcntl.h>
#include <errno.h>
#include "libmicrocai.h"
#define	 SERVER_SIDE
#include "kServer.hpp"

static void notice_mainthread()
{
	close(open("/tmp/monitor.socket",O_RDWR|O_CLOEXEC));
}

static ulong GenNextCode(ulong cur)
{
	return cur + rand();
}

static std::map<sockaddr_in,active_client>	cmap;
static std::deque<writedata>				writequeue;

typedef std::map<sockaddr_in,active_client>::iterator it_c;

static char blank[128]={0};
static char	BUFFER[ETHERMTU];

static int NeedWrite = POLLOUT & 0;
static void OnWrite(int sock)
{
	while(writequeue.size())
	{
		writedata wd = writequeue.front();

		switch(sendto(sock,wd.data,wd.length,MSG_DONTWAIT,(sockaddr*)&wd.addr,INET_ADDRSTRLEN))
		{
		case 0:
			return;
		case -1:
		if(errno==EINTR)
			continue;
		return;
		}
		writequeue.pop_front();
	}
	//TODO:写数据
	NeedWrite = 0;
}

static void	SendData(const void * buf,size_t len,active_client * to)
{
	writedata	wd;
	memcpy(wd.data,buf,len);
	wd.length = len;
	memcpy(&wd.addr , & to->sock_addr ,INET_ADDRSTRLEN);

	writequeue.push_back(wd);
	NeedWrite = POLLOUT;
}

static void OnNewConnection(int sock,sockaddr_in & addr)
{
	it_c it;
	if (memcmp(BUFFER, blank, 128) == 0)
	{
		active_client ac;
		active_client *pac;

		ac.last_active_time = time(&ac.last_active_time);
		ac.sock_addr = addr;
		ac.lastdata = NULL;
		it = cmap.insert(std::pair<sockaddr_in,active_client>(addr,ac)).first;
		pac = & (it->second);
		pac->login_code = ((uint64_t)pac);
		pac->current_code = pac->login_code;
		pac->passwd_token = pac->current_code << 32 | ~ pac->current_code ;
		pac->prev_code = 0 ;
		pac->mode = MODE_OK;
		pac->state = SYN_WAIT ;
		snprintf(BUFFER,ETHERMTU,"SB OK %lu",(ulong)pac->login_code);

		SendData(BUFFER,30,pac);

	}
}

static bool checkpasswd(const char * passwd, const char * Tpasswd, ulong code)
{
	return (atoll(Tpasswd) + code) == atoll(passwd);
}

static void Send_SB_OK(ulong pre, ulong cur, active_client * to)
{
	int len = snprintf(BUFFER, ETHERMTU, "SB OK %lu %lu", pre, cur);
	to->mode = MODE_NEED_PASSWD;
	SendData(BUFFER, len, to);
}
static void Send_SB_OK_OK(active_client * to)
{
	SendData("SB OK OK", 9, to);
}

static void MakeItChange( const char * IsADD,const char * build,const char * floor,const char * room)
{
	CString sqlstr;
	sqlstr.Format("insert into room_change (Is_ADD,RoomBuild,RoomFloor,RoomNum) "
			"values ('%s','%s','%s','%s')",IsADD, build, floor, room);
	ksql_run_query(sqlstr);
}

static void MakeItOffline(const char * build,const char * floor,const char * room)
{
	CString sql;
	sql.Format(
			"update room_list set IsEmpty=1,state=0,IsWhite=2 "
				"where IsWhite=0 and RoomBuild='%s' and RoomFloor='%s' and RoomNum='%s'",
			build, floor, room);
	ksql_run_query(sql);

	MakeItChange("0",build,floor,room);
}

static int On_ONLINE(char * command)
{
	if(strncmp(command,"ONLINE ",7))return 0;

	char * cmd;
	char *TOKPTR;

	strtok_r(command," ",&TOKPTR);

	do
	{
		// online IDTYPE,ID,NAME,房间,楼层,栋,国家,地址,发证机关
		cmd = strtok_r(0, ",", &TOKPTR);
		if (!cmd)
			break;
		if(atoi(cmd)==0)break;
		std::string Idtype(cmd);

		cmd = strtok_r(0, ",", &TOKPTR);
		if (!cmd)
			break;
		if(  strlen(cmd) < 6 )break;
		std::string ID(cmd);

		cmd = strtok_r(0, ",", &TOKPTR);
		if (!cmd)
			break;
		std::string name(cmd);

		cmd = strtok_r(0, ",", &TOKPTR);
		if (!cmd)
			break;
		if(atoi(cmd)==0)break;
		std::string room(cmd);

		cmd = strtok_r(0, ",", &TOKPTR);
		if (!cmd)
			break;
		std::string floor(cmd);
		if(atoi(cmd)==0)break;

		cmd = strtok_r(0, ",", &TOKPTR);
		if (!cmd)
			break;
		std::string build(cmd);

		cmd = strtok_r(0, ",", &TOKPTR);
		if (!cmd)
			break;
		std::string country(cmd);
		if(country=="N/A")
			country="CHN";

		cmd = strtok_r(0, ",", &TOKPTR);
		if (!cmd)
			break;
		std::string address(cmd);
		if(address=="N/A")
			address=="";

		cmd = strtok_r(0, ",", &TOKPTR);
		if (!cmd)
			break;
		std::string org(cmd);
		if(org=="N/A")
			org="";

		std::string  time;

		CString loginnum;

		formattime(time,GetCurrentTime());

		loginnum.Format("%1.1s%1.1s%02.2d%0.6s", build.c_str(),
				floor.c_str(), atoi(room.c_str()), ID.c_str() + ID.length() - 6);

		CString sqlstr;

		sqlstr.Format("update room_list set IsEmpty=0,state=0,LoginNum='%s',CustomerName='%s',IDtype='%s',"
				"ID='%s',live_address='%s',country='%s',org='%s',Time='%s',IP_ADDR='',MAC_ADDR='' where RoomBuild='%s' and RoomFloor='%s' and RoomNum='%s'",
				 loginnum.c_str(),name.c_str(),Idtype.c_str(),ID.c_str(),address.c_str(),
				 country.c_str(),org.c_str(), time.c_str(),
				 build.c_str(),floor.c_str(),room.c_str());

		ksql_run_query(sqlstr);

		MakeItChange("2",build.c_str(), floor.c_str(), room.c_str());


		InsertCustomerLog(build.c_str(), floor.c_str(), room.c_str(), name.c_str(),
				Idtype.c_str(), ID.c_str(), "0", "","" ,time.c_str());

		notice_mainthread();

		return 1;
	}while(false);
	return 0;
}

static int On_OFFLINE(char * command)
{
	if(strncmp(command,"OFFLINE ",8))return 0;

	char *cmd;
	char *TOKPTR;

	strtok_r(command," ",&TOKPTR);

	int ret=0;

	do {
		// offline 房间,楼层,栋
		cmd = strtok_r(0, ",", &TOKPTR);
		if (!cmd)
			break;
		std::string room(cmd);
		if (atoi(cmd) == 0)
			break;

		cmd = strtok_r(0, ",", &TOKPTR);
		if (!cmd)
			break;
		std::string floor(cmd);
		if (atoi(cmd) == 0 )
			if(!( floor == "0"))
					break;

		cmd = strtok_r(0, ", ", &TOKPTR);
		if (!cmd)
			break;
		std::string build(cmd);

		CString sqlstr;

		std::string  time;

		formattime(time,GetCurrentTime());

		sqlstr.Format("select CustomerName,IDtype,ID,IP_ADDR,MAC_ADDR"
			" from room_list where "
			"RoomBuild='%s' and RoomFloor='%s' and RoomNum='%s' ",
			build.c_str(),floor.c_str(),room.c_str());

		MYSQL_RES * res = ksql_query_and_use_result(sqlstr);

		MYSQL_ROW row = ksql_fetch_row(res);

		if(row)
		{
			InsertCustomerLog(build.c_str(),floor.c_str(),room.c_str(),row[0],row[1],row[2],"1",row[3]?row[3]:"",row[4]?row[4]:"",time.c_str());
		}

		ksql_free_result(res);

		MakeItOffline(build.c_str(),floor.c_str(),room.c_str());

		notice_mainthread();

		ret = 1;

	} while (false);
	return ret;
}

static int On_CHANGE(char * command)
{
	if(strncmp(command,"CHANGE ",7))return 0;

	int ret=0;

	char *cmd;
	char *TOKPTR;
	// change IDtype,ID,房间,楼层,栋

	strtok_r(command," ",&TOKPTR);

	do
	{
		cmd = strtok_r(0, ",", &TOKPTR);
		if (!cmd)
			break;
		if (atoi(cmd) == 0)
			break;
		CString Idtype(cmd);

		cmd = strtok_r(0, ",", &TOKPTR);
		if (!cmd)
			break;
		if (strlen(cmd) < 6)
			break;
		CString ID(cmd);

		cmd = strtok_r(0, ",", &TOKPTR);
		if (!cmd)
			break;
		CString room(cmd);
		if (atoi(cmd) == 0)
			break;

		cmd = strtok_r(0, ",", &TOKPTR);
		if (!cmd)
			break;
		CString floor(cmd);
		if (atoi(cmd) == 0)
			break;

		cmd = strtok_r(0, ",", &TOKPTR);
		if (!cmd)
			break;
		CString build(cmd);

		CString sql;
		sql.Format("select RoomBuild,RoomFloor,RoomNum,CustomerName,"
				"live_address,country,org,IP_ADDR,MAC_ADDR "
				"from room_list where ID='%s' and IDtype='%s' ",
				ID.c_str(), Idtype.c_str());
		MYSQL_RES * res = ksql_query_and_use_result(sql);

		MYSQL_ROW row = ksql_fetch_row(res);

		if(row)
		{

			std::string  time;

			formattime(time,GetCurrentTime());

			MakeItOffline(row[0], row[1 ], row[2]);

			InsertCustomerLog(row[0], row[1 ], row[2],row[3],Idtype.c_str(),ID.c_str(),"1",row[7]?row[7]:"",row[8]?row[8]:"",time.c_str());

			CString loginnum;

			loginnum.Format("%1.1s%1.1s%02.2d%0.6s", build.c_str(),
					floor.c_str(), atoi(room), ID.c_str() + ID.size() - 6);

			sql.Format("update room_list set IsEmpty=0,state=0,LoginNum='%s',CustomerName='%s',IDtype='%s',"
					"ID='%s',live_address='%s',country='%s',org='%s' where RoomBuild='%s' and RoomFloor='%s' and RoomNum='%s'",
					 loginnum.c_str(),row[3],Idtype.c_str(),ID.c_str(),row[4],
					 row[5],row[6], build.c_str(),floor.c_str(),room.c_str());

			ksql_run_query(sql);

			MakeItChange("2",build.c_str(), floor.c_str(), room.c_str());

			notice_mainthread();

			InsertCustomerLog(build.c_str(), floor.c_str(), room.c_str(),row[3],Idtype.c_str(),ID.c_str(),"0",row[7]?row[7]:"",row[8]?row[8]:"",time.c_str());

			ret = 1;
		}
		ksql_free_result(res);
	} while (0);
	return ret;
}


static struct _t_token_processor
{
	int (*proc)(char*);
} token_processor[] = {
	{ On_ONLINE },
	{ On_OFFLINE },
	{ On_CHANGE },
	{ 0 }
};


static void OnRead(int sock)
{
	char	* TOKPTR;
	sockaddr_in addr={0};
	addr.sin_family = AF_INET;
	socklen_t  addr_len = INET_ADDRSTRLEN;

	int recvd = recvfrom(sock,BUFFER,ETHERMTU,MSG_DONTWAIT,(sockaddr*)&addr,&addr_len);

	it_c it = cmap.find(addr);
	active_client * pac;

	if(it==cmap.end()) // new connection arrived
	{
		OnNewConnection(sock,addr);
	}else
	{

		pac = & it->second;

		if (pac->mode & MODE_OK  && pac->state == SYN_ESTABLISHED)
		{
			pac->mode = 0;

			if(pac->lastdata)
				delete [] pac->lastdata;

			pac->lastdata = new char[recvd+1];

			memcpy(pac->lastdata,BUFFER,recvd);
			log_printf(L_DEBUG_OUTPUT,"on recv %s\n",BUFFER);

			pac->lastdata_len = recvd;
			pac->prev_code = pac->current_code;
			pac->current_code = GenNextCode(pac->current_code);
			return Send_SB_OK(pac->prev_code,pac->current_code,pac);
		}

		char * cmd  = strtok_r(BUFFER, " ", &TOKPTR);

		if(pac->state != SYN_ESTABLISHED)
		{
			if(strncmp(BUFFER,"LOGIN",5)==0)
			{
				cmd = strtok_r(0, " ", &TOKPTR);
				if ((pac->mode & MODE_OK) && pac->login_code == atoll(cmd))
				{
					pac->state = SYN_ESTABLISHED;

					pac->mode = MODE_NEED_PASSWD;

					time(&pac->last_active_time);
					pac->prev_code = pac->current_code;
					char buf[128];
					memset(buf, 1, 128);
					pac->lastdata = NULL;

					return SendData(buf, 128, pac);
				}
			}
			cmap.erase(it); // baby, byebye!
			return;
		}

		if (strcmp(cmd, "PASSWD") == 0)
		{


			cmd = strtok_r(0, " ", &TOKPTR);
			if (checkpasswd(cmd, "123456", pac->current_code))
			{
				pac->mode = MODE_OK;

				if(!pac->lastdata)
				{
					return Send_SB_OK_OK(pac);
				}

				int i = 0;
				while(token_processor[i].proc)
				{
					if (token_processor[i].proc(pac->lastdata))
					{
						Send_SB_OK_OK(pac);
						break;
					}
					++i;
				}

				cmap.erase(it);
			}

		}

	}
}

static void CleanUpTimedOut()
{
	cmap.clear();
}

static void* server_thread(void*p)
{
	pollfd pfd;
	pfd.fd = (long) p;

	for (;;)
	{
		pfd.events = POLLIN | NeedWrite | POLLERR;
		switch (poll(&pfd, 1, 2500))
		{
		case 0:
			CleanUpTimedOut();
			break;
		case -1:
			if(errno == EINTR)
				continue;
			return 0;
		}
		if (pfd.revents & POLLOUT)
		{
			OnWrite(pfd.fd);
		}
		if(pfd.revents& POLLIN)
		{
			OnRead(pfd.fd);
		}
		if(pfd.revents & POLLERR)
			break;
	}

	return 0;
}

extern "C" int __module_init(struct so_data*so)
{

	std::cout << "\t绑定到端口 " << SERVER_PORT << "......";
	long udp;
	int optval;
	sockaddr_in add =
	{ 0 };

	add.sin_family = AF_INET;
	add.sin_port = htons(SERVER_PORT);

	pthread_t thread;
	do
	{
		udp = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

		if (udp < 0)
			break;
		setsockopt(udp, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
		if (bind(udp, (sockaddr*) &add, INET_ADDRSTRLEN) < 0)
		{
			std::cout << "失败!" << std::endl;
			break;
		}
		std::cout << "OK" << std::endl;
		return pthread_create(&thread, 0, server_thread, (void*)udp);
	} while (false);
	return 1;
}

extern "C" int	so_can_unload(  )
{
	return 0;
}

char module_name[] = "精诚管理模式";

/*  下面是本模块的 README 文档，同时也是本模块客户端的开发指南
 ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


 kserver  提供了另一种管理服务器的选择，同时也是高级管理模式。浏览器模式将缺少一些伟大的功能。

 kserver 支持多个客户访问，同时加密访问。kserver 使用动态密匙， 攻击者更本不可能成功。

 一下 C 代表 客户（Client），S 代表服务程序 （Server）

 协议：	 起初，C 发出 包含 128 个 0 的数据包来进行登录通知. S 通过回应 登录码回应。
		 如果 S 指定时间内没有回应，就发送 256 个 0 代码重试。
		 S 如果已经回应，但是再次收到 256 个 0 ，则 可以再次回应 登录码。
		 S 只允许重试 5 次。

		 C 发出 LOGIN 相同登录码 表示收到。
		 S 回应 128 个 1 表示连接建立，要求登录
		 C 如果没有收到 128 个 1 可以再次发送登录码。
		 S 只允许重试 3 次。

		 现在，进入文本通信模式。
		 所有的重复请求或回答都要以  REP：   打头，以便和非重复应答区别开来。
		 S 的所有回复都以 SB 打头 (SB 们，破解啊)

		 S 选择 登录码 做为密匙，加密密码后 发送加密后的密码ACSII串 登录。
		 通信过程  PASSWD *
		 S 同样回应以  SB OK 附带旧登录码和新的登录码。
		 C 没有收到就要以 REP: PASSWD * 重新发送
		 S 同样回应以  SB OK 附带旧登录码和新的登录码。
		 S 只允许重试 3 次。

		 以后每次通信都要求先 C 发送命令，接着S 回复 SB OK 附带旧登录码和新的登录码 ，C
		 再次发送用新的登录码加密
		 后的代码。 服务器回应 SB OK OK 完成命令传送。

		 总之，每次通信过程都要改变密匙，使得伪装数据包成为不可能的事情，除非知道密码。
		 服务器回应的数据是不加密的，所以没有改变密匙的必要。密匙的改变只在于C 向 S 发送命令

		 目前定义的命令有 ONLINE OFFLINE CHANGE

 */
