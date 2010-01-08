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
#include <vector>
#include <deque>
#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/syslog.h>
#include <netinet/in.h>
#include <net/ethernet.h>

//We need md5
#include <fcntl.h>
#include <errno.h>
#include "libdreamtop.h"
#define	 SERVER_SIDE
#include "kServer.hpp"

static void notice_mainthread()
{
	close(open("/tmp/monitor.socket",O_RDWR|O_CLOEXEC));
}

static std::vector<active_client*>	activeclients;

static char	BUFFER[ETHERMTU];

static bool checkpasswd(const char * passwd, const char * Tpasswd, int32_t code)
{
	return (atoi(Tpasswd) + code) == atoi(passwd);
}

static void Send_SB_OK(int32_t code, active_client * to)
{
	int len = snprintf(BUFFER, ETHERMTU, "SB OK %d", code);
	write(to->sockfd,BUFFER,len);
}

static void Send_SB_OK_OK(active_client * to)
{
	write(to->sockfd,"SB OK OK", 9);
}

static void MakeItChange(int roomid, int action )
{
	MYSQL_RES * res;
	MYSQL_ROW row;
	CString sqlstr;


	sqlstr.Format("select nIndex from roomer_list where"
			" RoomId='%d' order by nIndex desc",roomid);
	res = ksql_query_and_use_result(sqlstr);
	sqlstr.Format("");
	row = ksql_fetch_row(res);
	if(row && row[0])
	{
		sqlstr.Format("insert into room_change (RoomerId,ActionType) "
				"values ('%s','%d')",row[0], action);
	}
	ksql_free_result(res);

	ksql_run_query(sqlstr);
}


static int GetRoomID(const char * build,const char * floor,const char * room)
{
	MYSQL_RES * res;
	MYSQL_ROW row;
	CString sql;
	int nindex ;
	sql.Format(
			"select nIndex from room_list where RoomBuild='%s' and RoomFloor='%s' and RoomNum='%s'",
			build, floor, room);
	res = ksql_query_and_use_result(sql);
	if((row = ksql_fetch_row(res)))
	{
		nindex =  atoi(row[0]);
	}else nindex = -1;
	ksql_free_result(res);
	return nindex;
}

static int On_ONLINE(char * command)
{
	if(strncmp(command,"ONLINE ",7))return 0;

	char * cmd;
	char *TOKPTR;

	int nIndex;

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

		// 获得 RoomId
		nIndex = GetRoomID(build.c_str(),floor.c_str(),room.c_str());

		if(nIndex>0)
		{
			// 更新 rooom_list 的 count 值
			sqlstr.Format("update room_list set RoomerCount=RoomerCount+1 where nIndex=%d",nIndex);
			ksql_run_query(sqlstr);

			//插入新的用户

			sqlstr.Format("insert into roomer_list "
					"(CustomerName,ID,IDtype,IP_ADDR,MAC_ADDR,RoomId,Time,live_address,country,org,LoginNum) "
					"values ('%s','%s','%s',null,null,'%d','%s','%s','%s','%s','%s')",
					name.c_str(),ID.c_str(),Idtype.c_str(),nIndex,time.c_str(),
					address.c_str(),country.c_str(),org.c_str(),loginnum.c_str());
			ksql_run_query(sqlstr);

			MakeItChange(nIndex,3);

			syslog(LOG_NOTICE,"从金城登记 :%s ...ok\n",name.c_str());

			notice_mainthread();

			InsertCustomerLog(build.c_str(),floor.c_str(),room.c_str(),name.c_str(),Idtype.c_str(),ID.c_str(),"0","","",time.c_str());
		}
		return 1;
	}while(false);
	return 0;
}

static int On_OFFLINE(char * command)
{
	if(strncmp(command,"OFFLINE ",8))return 0;

	char *cmd;
	char *TOKPTR;

	MYSQL_RES * res,*tres;
	MYSQL_ROW row,trow;

	strtok_r(command," ",&TOKPTR);

	int ret=0;

	do {
		// offline room,floor,build

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
		if (atoi(cmd) == 0)
			break;

		cmd = strtok_r(0, ",", &TOKPTR);
		if (!cmd)
			break;
		std::string build(cmd);

		CString sqlstr;

		std::string  time;

		formattime(time,GetCurrentTime());

		//找出这房间，退干净人。

		sqlstr.Format("select l.`nIndex`  from room_list  l "
				"  where l.`RoomBuild` = '%s' and l.`RoomFloor` = '%s' and l.`RoomNum` = '%s' ",
				build.c_str(),floor.c_str(),room.c_str());

		res =ksql_query_and_use_result(sqlstr);
		while((row=ksql_fetch_row(res)))
		{
			sqlstr.Format("update room_list set RoomerCount=0 where nIndex='%s'",row[0]);
			ksql_run_query(sqlstr);

			sqlstr.Format("select nIndex,CustomerName,IDtype,ID,IP_ADDR,MAC_ADDR from roomer_list where RoomId='%s'",row[0]);

			tres = ksql_query_and_use_result(sqlstr);
			while((trow=ksql_fetch_row(tres)))
			{
				sqlstr.Format("insert  into room_change (RoomerId,ActionType) values ('%s',2)",trow[0]);
				ksql_run_query(sqlstr);
				//记录日志
				syslog(LOG_NOTICE,"从金城退掉 :%s \n",trow[1]);

				InsertCustomerLog(build.c_str(),floor.c_str(),room.c_str(), trow[1], trow[2],trow[3],
						 "1", trow[4] ? trow[4] : "",
					trow[5] ? trow[5] : "", time.c_str());

			}ksql_free_result(tres);
			ret = 1;
		}
		ksql_free_result(res);
		notice_mainthread();
		ret = 1;
	} while (false);
	return ret;
}

static int On_CHANGE(char * command)
{
	if(strncmp(command,"CHANGE ",7))return 0;

	int ret=0;

	CString sql;
	char *cmd;
	char *TOKPTR;
	std::string  time;

	std::string name,address,country,org,ip,mac;


	MYSQL_RES * res;
	MYSQL_ROW row;
	// change IDtype,ID,房间,楼层,栋


	strtok_r(command," ",&TOKPTR);

	do
	{
		cmd = strtok_r(0, ",", &TOKPTR);
		if (!cmd)
			break;
		if (atoi(cmd) == 0)
			break;
		std::string Idtype(cmd);

		cmd = strtok_r(0, ",", &TOKPTR);
		if (!cmd)
			break;
		if (strlen(cmd) < 6)
			break;
		std::string ID(cmd);

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
		if (atoi(cmd) == 0)
			break;

		cmd = strtok_r(0, ",", &TOKPTR);
		if (!cmd)
			break;
		std::string build(cmd);


		// 获得这家伙的详细信息

		CString loginnum;

		formattime(time,GetCurrentTime());

		loginnum.Format("%1.1s%1.1s%02.2d%0.6s", build.c_str(),
				floor.c_str(), atoi(room.c_str()), ID.c_str() + ID.size() - 6);

		sql.Format("SELECT r.`CustomerName`, r.`IP_ADDR`, r.`MAC_ADDR`, r.`live_address`, r.`country`, r.`org`"
				" FROM roomer_list r , room_list l"
				" where r.`ID`='%s' and r.`IDtype`='%s' and l.nIndex=r.RoomId ",
				ID.c_str(), Idtype.c_str());

		syslog(LOG_NOTICE,"从金城换房 ， sql : %s\n",sql.c_str());

		res = ksql_query_and_use_result(sql);

		if((row=ksql_fetch_row(res)))
		{
			if(row[0])
				name = row[0];
			if(row[1])
				ip=row[1];
			if(row[2])
				mac = row[2];
			if(row[3])
				address =row[3];
			if(row[4])
				country = row[4];
			if(row[5])
				org = row[5];
		}

		ksql_free_result(res);

		// 找出这家伙的所有旧的房间，退掉

		sql.Format("select r.`RoomId`,r.`nIndex`,r.`CustomerName`,r.`IP_ADDR`,r.`MAC_ADDR`,"
				"l.`RoomBuild`, l.`RoomFloor`, l.`RoomNum` "
				"from roomer_list r , room_list l where IDtype='%s' and ID='%s' and l.nIndex=r.RoomId",
				Idtype.c_str(),ID.c_str());
		res =ksql_query_and_use_result(sql);
		while((row=ksql_fetch_row(res)))
		{
			sql.Format("update room_list set RoomerCount=RoomerCount-1 where nIndex='%s' and RoomerCount>0",row[0]);
			ksql_run_query(sql);

			sql.Format("insert into room_change (RoomerId,ActionType) values ('%s',2)",row[1]);
			ksql_run_query(sql);

			//记录日志
			InsertCustomerLog(row[5],row[6],row[7],row[2],Idtype.c_str(),ID.c_str(),"1",row[3]?row[3]:"",row[4]?row[4]:"",time.c_str());

		}ksql_free_result(res);

		//好，现在，住到新的房间里去

		// 获得 RoomId
		int nIndex = GetRoomID(build.c_str(),floor.c_str(),room.c_str());

		if(nIndex>0)
		{
			// 更新 rooom_list 的 count 值
			sql.Format("update room_list set RoomerCount=RoomerCount+1 where nIndex=%d",nIndex);
			ksql_run_query(sql);

			//插入新的用户
			sql.Format("insert into roomer_list "
					"(CustomerName,ID,IDtype,IP_ADDR,MAC_ADDR,RoomId,Time,live_address,country,org,LoginNum) "
					"values ('%s','%s','%s',null,null,'%d','%s','%s','%s','%s','%s')",
					name.c_str(),ID.c_str(),Idtype.c_str(),nIndex,time.c_str(),
					address.c_str(),country.c_str(),org.c_str(),loginnum.c_str());

			ksql_run_query(sql);

			MakeItChange(nIndex,3);

			notice_mainthread();

			InsertCustomerLog(build.c_str(),floor.c_str(),room.c_str(),name.c_str(),Idtype.c_str(),ID.c_str(),"0","","",time.c_str());
		}
		ret=1;
	} while (0);
	return ret;
}


static struct _t_token_processor
{
	int (*proc)(char*);
} token_processor[] = {
	{ On_ONLINE },
	{ On_CHANGE },
	{ On_OFFLINE },
	{ 0 }
};


static int OnRead(active_client * pac)
{
	int sock = pac->sockfd;
	char	* TOKPTR;
	char * cmd;

	bzero(BUFFER,sizeof(BUFFER));
	int recvd = read(sock,BUFFER,ETHERMTU);

	if(pac->lastdata)
	{
		// 接收 密码后开始执行代码。
		cmd = strtok_r(BUFFER," ",&TOKPTR);
		if (strcmp(cmd, "PASSWD") == 0)
		{
			if (checkpasswd(BUFFER+7, "123", pac->passwd_token))
			{
				int i = 0;
				while (token_processor[i].proc)
				{
					if (token_processor[i].proc(pac->lastdata))
					{
						Send_SB_OK_OK(pac);
						break;
					}
					++i;
				}
			}//else
//			log_printf(L_DEBUG_OUTPUT,"passwd check fail, passwd token is %d!\n",pac->passwd_token);
		}
		return 1;
	}else
	{
		pac->lastdata_len = recvd;
		pac->lastdata[recvd] = 0;
		memcpy(pac->lastdata,BUFFER,recvd>512?512:recvd);
		// 发送加密令牌
		Send_SB_OK(pac->passwd_token,pac);
		syslog(LOG_NOTICE,"%s\n",pac->lastdata);
		return 0;
	}
}

static void* server_thread(void*p)
{
	int listensocket = (long) p;
	epoll_event epev;
	epev.events = EPOLLIN|EPOLLERR|EPOLLRDHUP;
	epev.data.ptr = NULL;

	int epfd ,ret;
	epfd = epoll_create(10);

	epoll_ctl(epfd,EPOLL_CTL_ADD,listensocket,&epev);

	for (;;)
	{
		ret = epoll_wait(epfd, &epev, 1, 10000);

		if (ret == -1)
			if (errno == EINTR || errno == EAGAIN)
				continue;
		if (ret == -1)
		{
			exit(0); // restart , haha
		}

		if (ret == 0)
		{
			continue;
		}

		for (int i = 0; i < ret; ++i)
		{
			if (epev.events & (EPOLLHUP|EPOLLERR|EPOLLRDHUP ))
			{
				active_client * pac = (active_client*) epev.data.ptr;
				if (pac)
				{
					epoll_ctl(epfd,EPOLL_CTL_DEL,pac->sockfd,0);
					close(pac->sockfd);
					delete pac;
				}
				else
					exit(0);
			}
			if (epev.events & EPOLLIN)
			{
				if (epev.data.ptr)
				{
					active_client * pac = (active_client*) epev.data.ptr;

					if (OnRead(pac) != 0)
					{
						epoll_ctl(epfd,EPOLL_CTL_DEL,pac->sockfd,0);
						close(pac->sockfd);
						delete pac;
					}
				}
				else
				{
					active_client * pac = new active_client;
					epev.data.ptr = pac;
					epev.events = EPOLLIN|EPOLLERR|EPOLLRDHUP|EPOLLHUP;
					pac->sockfd = accept(listensocket,0,0);
					epoll_ctl(epfd,EPOLL_CTL_ADD,pac->sockfd,&epev);
					activeclients.insert(activeclients.end(),pac);
					continue;
				}
			}
		}
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
		udp = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

		if (udp < 0)
			break;
		setsockopt(udp, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
		if (bind(udp, (sockaddr*) &add, INET_ADDRSTRLEN) < 0)
		{
			std::cout << "失败!" << std::endl;
			break;
		}
		listen(udp,10);
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
