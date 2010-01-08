/*
 * auto_update.cpp
 *
 *  Created on: 2009-9-10
 *      Author: cai
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <list>
#include <unistd.h>
#include <sys/fcntl.h>
#include <sys/mman.h>
#include <sys/syslog.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <dirent.h>
#include "libdreamtop.h"
#include "md5.h"

struct md5_digst{
	u_char hash[16];
	md5_digst(u_char md5[16],const char * _name)
	{
		memcpy(hash,md5,16);
		strncpy(name,_name,256);
	}
	char name[256];
};

static u_char md_exe[16];
static std::list<md5_digst> modules;

static char dec2hex(int dec)
{
	switch (dec)
	{
	case 0 ... 9:
		return dec + '0';
	case 10 ... 15:
		return dec - 10 + 'a';
	default:

		return 0;
	}
}

static void formatMD5(u_char hash[16],char strout[33])
{
	for(int i=0;i<16;i++)
	{
		strout[i*2] = dec2hex(hash[i] & 0xF);
		strout[i*2+1] = dec2hex((hash[i] >> 4) & 0xF);
	}
	strout[32] = '0';
}
static int GenTrunk(char * out, u_char hash[16], const char * file,const char * truck)
{
	char md5[33];
	formatMD5(hash,md5);
	return sprintf(out,"file:%s\r\nMD5:%s\r\ntruck:%s\r\n\r\n",file,md5,truck);
}

static in_addr_t gethostipbyname(const char * updateserver)
{
	struct hostent hosts[2]={{0}};
	struct hostent *phosts=0;
	char buf[1024];
	int perr;

	gethostbyname2_r(updateserver,AF_INET,hosts,buf,1024,&phosts,&perr);
	if(!phosts) return 0;

	return  *((in_addr_t*)(phosts->h_addr_list[0]));

}

static void firstcall()
{
	int e;
	struct stat st;
	struct dirent *dt;
	// 计算 MD5 值
	// 首先计算自己 /usr/local/bin/monitor。
	char excutable[2096]=EXCUTE_PATH;

	void * map_exe;

	strcat(excutable,PACKAGE_NAME);
	e = open(excutable,O_CLOEXEC|O_RDONLY);
	if(e < 0 ) return ;
	fstat(e,&st);
	map_exe = mmap(NULL,st.st_size,PROT_READ,MAP_PRIVATE,e,0);
	close(e);
	if(map_exe != MAP_FAILED)
	{
		Computehash((u_char*)map_exe,st.st_size,md_exe);
		munmap(map_exe,st.st_size);
	}
	// 进入循环，分别计算各个模块的MD5值
	DIR * dir = opendir(MODULES_PATH);
	if(!dir)
	{
		syslog(LOG_WARNING,"没有找到扩展模块");
		return ;
	}

	while( (dt = readdir(dir)))
	{
		u_char md5[16];
		strcpy(excutable,MODULES_PATH);
		strcat(excutable,dt->d_name);

		e = open(excutable,O_CLOEXEC|O_RDONLY);
		if(e < 0 ) return ;
		fstat(e,&st);
		map_exe = mmap(NULL,st.st_size,PROT_READ,MAP_PRIVATE,e,0);
		close(e);
		if(map_exe != MAP_FAILED)
		{
			Computehash((u_char*)map_exe,st.st_size,md5);
			munmap(map_exe,st.st_size);
			modules.insert(modules.end(),md5_digst(md5,dt->d_name));
		}
	}
}

int Check_update(const char * updateserver,const char * update_trunk)
{
	static int sock=-1;
	static int last_state=0;
	static sockaddr_in addr={0};

	if(sock==-1)
	{
		sock = socket(AF_INET,SOCK_DGRAM,0);
		if(sock>0)
			firstcall();
		return 0;
	}else if (sock == 0)
	{
		// is updating !
		return 0;
	}
	int s;
	char buffer[1500];

	switch(last_state)
	{
	case 0: // 第二次调用，故而需要DNS解析
		addr.sin_family = AF_INET;
		addr.sin_port = 32088;
		addr.sin_addr.s_addr = gethostipbyname(updateserver);
		if(addr.sin_addr.s_addr==0)		break;
		syslog(LOG_NEWS,"dns resove:%s = %s \n",updateserver,inet_ntoa(addr.sin_addr));

		connect(sock,(sockaddr*)&addr,INET_ADDRSTRLEN);

		last_state = 1;
		break;
	case 1: // 第3次调用，DNS解析成功，开始查询主程序是否要更新
		s =GenTrunk(buffer,md_exe,PACKAGE_NAME,update_trunk);
		send(sock,buffer,s,0);
		last_state = 2;
		break;
	case 2: //第4次调用，接受服务端信息
		break;
	}
	return 0;
}
/*
 *  协议 :
 *  file : %file \r\n
 *  MD5:%md5 \r\n
 *  trunk : %truck \r\n\r\n
 *
 *  服务回复
 *
 *  file : %file \r\n
 *  MD5:%md5 \r\n
 *  trunk : %truck \r\n\r\n
 *
 */
