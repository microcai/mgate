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
#include <iostream>
#include <string>
#include <vector>
#include <list>
#include <map>
#include <iterator>

#include <sys/poll.h>
#include <netinet/in.h>
#include <net/ethernet.h>
#include <libmicrocai.h>
#define	 SERVER_SIDE
#include "kServer.hpp"


std::map<struct sockaddr_in,struct active_client>	cmap;

typedef std::map<struct sockaddr_in,struct active_client>::iterator it_c;


static void OnRead(int sock)
{
	//TODO:读取数据
	char *_buf = new char[ETHERMTU];

	sockaddr_in addr={0};
	addr.sin_family = AF_INET;
	socklen_t  addr_len = INET_ADDRSTRLEN;
	recvfrom(sock,_buf,ETHERMTU,MSG_DONTWAIT,(sockaddr*)&addr,&addr_len);

	it_c it = cmap.find(addr);



	delete[]_buf;
}
static int NeedWrite = POLLOUT;
static void OnWrite(int sock)
{
	//TODO:写数据
	NeedWrite = 0;
	sockaddr_in addr={0};

	cmap.find(addr);

}
static void CleanUpTimedOut()
{
	//TODO:清理超时的连接

}
static void* server_thread(void*p)
{
	pollfd pfd;
	pfd.fd = (long) p;
	pthread_exit(0);
	for (;;)
	{
		pfd.events = POLLIN | NeedWrite | POLLERR;
		switch (poll(&pfd, 1, -1))
		{
		case 0:
			CleanUpTimedOut();
			break;

		case -1:
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
		pthread_create(&thread, 0, server_thread, (void*)udp);
		return 0;
	} while (false);
	return 1;
}
char module_name[] = "高级管理模式";
char _depmod_[] = "kroom.so"; //由于要访问 sql数据库，sql数据库在 kroom.so 里面初始化的


/*  下面是本模块的 README 文档，同时也是本模块客户端的开发指南
 ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


 kserver  提供了另一种管理服务器的选择，同时也是高级管理模式。浏览器模式将缺少一些伟大的功能。

 kserver 支持多个客户访问，同时加密访问。kserver 使用动态密匙， 攻击者更本不可能成功。

 一下 C 代表 客户（Client），S 代表服务程序 （Server）

 协议：	 起初，C 发出 包含 128 个 0 的数据包来进行登录通知. S 通过回应 登录码回应。
		 如果 S 指定时间内没有回应，就发送 256 个 0 代码重试。
		 S 如果已经回应，但是再次收到 256 个 0 ，则 可以再次回应 登录码。
		 S 只允许重试 5 次。

		 C 发出 相同登录码 表示收到。
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

		 目前定义的命令有 GetHotelInfo SetHotelInfo

 */
