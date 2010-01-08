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
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/fcntl.h>
#include <sys/mman.h>
#include <sys/syslog.h>
#include <sys/socket.h>
#include <sys/syslog.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include "libdreamtop.h"
#include "md5.h"

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
	strout[32] = '\0';
}

static int GenTrunk(char * out, u_char hash[16], const char * file,const char * trunk)
{
	char md5[33];
	formatMD5(hash,md5);
	return sprintf(out,"File:%s\r\nMD5:%s\r\nTrunk:%s\r\n\r\n",file,md5,trunk);
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

struct md5_digst{
	/* 0 un send
	 * 1 send , need replay
	 * 2 verifyed, no need to update
	 * 3 set to be updated
	 */

	int  sendstate;
	char name[256];
	u_char hash[16];
	md5_digst(u_char md5[16],const char * _name)
	{
		memcpy(hash,md5,16);
		strncpy(name,_name,256);
		sendstate = 0;
	}

};

static u_char md5_exe[16];
static int	  exe_need_update=0;
static std::list<md5_digst> modules;
static std::list<std::string> modules_add;

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
	e = open(excutable, O_CLOEXEC | O_RDONLY);
	if (e > 0)
	{
		fstat(e, &st);
		map_exe = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, e, 0);
		close(e);
		if (map_exe != MAP_FAILED)
		{
			Computehash((u_char*) map_exe, st.st_size, md5_exe);
			munmap(map_exe, st.st_size);
		}
	}
	// 进入循环，分别计算各个模块的MD5值
	DIR * dir = opendir(MODULES_PATH);
	if(!dir)
	{
		syslog(LOG_WARNING,"没有找到扩展模块");
		return ;
	}

	modules.clear();
	modules_add.clear();

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
	closedir(dir);
}
// return -1 if no more call needed, 0 if want a timedout recall , 1 if want recall next time
int Check_update(const char * updateserver,const char * update_trunk)
{
	static int sock=-1;
	static int last_state=1;
	static sockaddr_in addr={0};
	static char buffer[1500];
	static int	need_update=0;
	static pid_t pid;
	FILE	*upshscript=NULL;
	std::list<md5_digst>::iterator it;

	struct timeval tv;

	int s;
	char	file[256],md5[256],trunk[256];

	switch(last_state)
	{
	case 0: // 无须调用，正在update
		if (waitpid(pid, &s, WNOHANG))
		{
			if (WEXITSTATUS(s) != 0)
			{
				syslog(LOG_WARNING,"script failed to execute\n");
				last_state = 1;
			}
			else
			{
				syslog(LOG_NEWS,"script exited with %d\nRESTARTING!!!!\n",
						WEXITSTATUS(s));
				exit(0);
			}
		}
		return -1;
	case 1: //首次调用，需要计算 MD5 值
		firstcall();
		last_state = 2;
		break;
	case 2: // 第二次调用，故而需要DNS解析
		addr.sin_family = AF_INET;
		addr.sin_port = htons(32088);
		addr.sin_addr.s_addr = gethostipbyname(updateserver);
		if (addr.sin_addr.s_addr == 0)break;

		syslog(LOG_NOTICE,"dns resove:%s = %s \n",updateserver,inet_ntoa(addr.sin_addr));

		sock = socket(AF_INET,SOCK_DGRAM,0);
		if(sock < 0) break; // 下次继续啊
		connect(sock,(sockaddr*)&addr,INET_ADDRSTRLEN);
		last_state = 3;
		tv.tv_sec = 1;
		tv.tv_usec = 500;

		setsockopt(sock,SOL_SOCKET,SO_RCVTIMEO,(char*)&tv,sizeof(tv));

		break;
	case 3: // 第3次调用，DNS解析成功，开始查询主程序是否要更新
		s = GenTrunk(buffer,md5_exe,PACKAGE_NAME,update_trunk);
		send(sock,buffer,s,0);
		last_state = 4;
		break;
	case 4: //第4次调用，接受服务端的消息。是否主程序需要更新
		last_state = 3;
		/*
		 * 哈哈，如果下面我不改成 这样不就自动重新发送了？
		 */
		bzero(buffer,sizeof(buffer));
		// 没有数据?
		/* 很好，现在，可以判断是否要更新主程序了*/

		do{
			if (recv(sock, buffer, sizeof(buffer),0) <= 0)
			{
				static int tryed = 0;
				tryed ++;
				if(tryed == 2)
				{
					tryed = 0;
					close(sock);
					last_state = 2;
					return -1;
				}
				break;
			}

			bzero(file,sizeof(file));
			bzero(md5,sizeof(md5));
			bzero(trunk,sizeof(trunk));
			sscanf(buffer, "File:%s\r\nMD5:%s\r\nTrunk:%s", file, md5, trunk);

			if (strcmp(file, "monitor"))
				break;
			if (strcmp(trunk, update_trunk))
				break;
			char	md55[256];

			formatMD5(md5_exe,md55);

			if (strcmp(md5, md55)) // 居然不一样诶
			{
				need_update = exe_need_update = 1;
				syslog(LOG_NOTICE,"%s set to be updated",file);
			}
			last_state = 5;
		}while(0);
		break;
	case 5: //发送余下的模块MD5值并接收，每次发送一个模块，并接收一个模块，对于要更新的，标记一下。
		bzero(buffer,sizeof(buffer));
		if (recv(sock, buffer, sizeof(buffer), MSG_DONTWAIT) > 0)
		{
			bzero(file,sizeof(file));
			bzero(md5,sizeof(md5));
			bzero(trunk,sizeof(trunk));
			sscanf(buffer, "File:%s\r\nMD5:%s\r\nTrunk:%s", file, md5, trunk);

			if (strcmp(trunk, update_trunk))
				break;
			char	md55[256];

			// find the one that I send
			for(it = modules.begin();it!=modules.end();++it)
			{
				if (strcmp(it->name, file) == 0 && it->sendstate == 1)
				{
					formatMD5(it->hash,md55);
					it->sendstate = strcmp(md5, md55) ? 3 : 2;
					if(it->sendstate ==3)
						syslog(LOG_NOTICE,"%s set to be updated",file);
					need_update |= (it->sendstate == 3);
					break;
				}
			}
		}
		// 发送一个吧，嘿嘿
		bzero(buffer,sizeof(buffer));
		for( it = modules.begin();it!=modules.end();++it)
		{
			if (it->sendstate == 0)
			{
				it->sendstate = 1;
				s = GenTrunk(buffer,it->hash,it->name,update_trunk);
				send(sock,buffer,s,0);
				break;
			}
		}
		last_state = it == modules.end() ? 6 : 5;
		break;
	case 6: //全部发送完毕，发送结束发送标志
#define	THAT_IS_ALL "That's All\r\n"
		send(sock,THAT_IS_ALL,sizeof(THAT_IS_ALL),0);
		last_state = 7;
		break;
	case 7: // 接受服务器的额外信息， 每次一个，直到服务器发送结束标志
		bzero(buffer,sizeof(buffer));
		if (recv(sock, buffer, sizeof(buffer), MSG_DONTWAIT) > 0)
		{
			if (strcmp(buffer, THAT_IS_ALL) == 0)
			{
				close(sock);
				last_state = 8;
				break;
			}
			bzero(file,sizeof(file));
			bzero(md5,sizeof(md5));
			bzero(trunk,sizeof(trunk));
			sscanf(buffer, "File:%s\r\nMD5:%s\r\nTrunk:%s", file, md5, trunk);

			if(strlen(file) && strlen(md5) && strlen(trunk))
			{
				std::list<std::string>::iterator it;

				for(it=modules_add.begin();it!=modules_add.end();++it)
				{
					if(it->compare(file))
					{
						break;
					}
				}

				if(it!=modules_add.end())
				{
					// 当中没有
					modules_add.push_back(file);
					need_update = 1;
				}
				bzero(buffer,sizeof(buffer));
				s = sprintf(buffer,"Got %s\r\n",file);
				send(sock,buffer,s,0);
			}
		}else{
			static int tryed = 0;
			tryed ++ ;
			if (tryed > 3)
			{
				// 接收不到，所以需要重新解析地址。
				close(sock);
				last_state = 2;
				tryed = 0;
			}
		}
		break;
	case 8: //  服务器已经发送结束标志。开始解析
		// 生成文件

		if(need_update)
		{
			srand(time(0));
			// 文件头和版权声明以及不要编辑的警告

			upshscript = fopen("/tmp/monitor_update","w");
			fputs("#! /bin/sh\n\n",upshscript);
			fprintf(upshscript,"#\tCopyright 2009-%d kingstone,Inc\n\n",2009);
			fprintf(upshscript,"#\tThis file is generated by %s, It is used to auto update\n",PACKAGE_NAME);
			fprintf(upshscript,"#\tthe %s it self\n#####DO NOT EDIT THIS FILE!#####\n\n\n\n",PACKAGE_NAME);

			// 首先设置好变量
			fprintf(upshscript,"#设置好环境变量\n");
			fprintf(upshscript,"set -x \n");
			fprintf(upshscript,"RM=\"rm -rf\"\n");
			fprintf(upshscript,"#临时下载文件夹\n");
			fprintf(upshscript,"DOWNTMP=\"/tmp/monitor_update.%d\"\n",rand());
			fprintf(upshscript,"#模块文件夹\n");
			fprintf(upshscript,"MODULES_PATH=\"%s\"\n",MODULES_PATH);
			fprintf(upshscript,"#程序文件夹\n");
			fprintf(upshscript,"BIN_PATH=\"%s\"\n",EXCUTE_PATH);
			fprintf(upshscript,"#下载服务器\n");
			fprintf(upshscript,"update_server=\"%s\"\n",updateserver);
			fprintf(upshscript,"#版本分支\n");
			fprintf(upshscript,"update_trunk=%s\n",update_trunk);
			fprintf(upshscript,"#程序名字\n");
			fprintf(upshscript,"monitor_bin=%s\n\n",PACKAGE_NAME);

			// 下载程序文件
			if(exe_need_update)
			{
				fprintf(upshscript,"mkdir -p ${DOWNTMP}/bin\n\n");
				fprintf(upshscript,"#下载程序文件\n");
				fprintf(upshscript,"echo 正在下载 ${monitor_bin} \n");
				fprintf(upshscript,"wget \"http://${update_server}/monitor_update/${update_trunk}/bin/${monitor_bin}\" \\\n\t-O ${DOWNTMP}/bin/${monitor_bin} -o /dev/null\n");
				fprintf(upshscript,"#下载程序成功否？\n");
				fprintf(upshscript,"if [ $? = 0 ] ; then \n");

				fprintf(upshscript,"#安装程序\n");
				fprintf(upshscript,"\tinstall -c  \"${DOWNTMP}/bin/${monitor_bin}\" \"${BIN_PATH}\"\n");
				fprintf(upshscript,"else\n\texit -1\nfi\n\n");
			}
			//下载模块文件
			fprintf(upshscript,"mkdir -p ${DOWNTMP}/modules\n\n");

			for(it=modules.begin();it!=modules.end();++it)
			{
				if(it->sendstate == 3)
				{
					fprintf(upshscript,"#下载模块文件\n");
					fprintf(upshscript,"echo 正在下载 %s \n",it->name);
					fprintf(upshscript,"wget \"http://${update_server}/monitor_update/${update_trunk}/modules/%s\" \\\n\t-O \"${DOWNTMP}/modules/%s\" -o /dev/null \n",it->name,it->name);
					fprintf(upshscript,"#下载模块文件成功否？\n");
					fprintf(upshscript,"if [ $? = 0 ] ; then \n");
					fprintf(upshscript,"#安装模块文件\n");
					fprintf(upshscript,"\tinstall -c  \"${DOWNTMP}/modules/%s\" \"${MODULES_PATH}\"\n",it->name);
					fprintf(upshscript,"# else 模块嘛，更新失败也是可以理解的啦，哈哈\nfi\n\n");
				}
			}
			last_state = 10;

			fprintf(upshscript,"echo 更新结束，清理临时文件夹\n");
			fprintf(upshscript,"rm -rf ${DOWNTMP}\n");

			fprintf(upshscript,"# 运行结束了，诶，退出就可以了吧?\n\nexit 0");
			fclose(upshscript);
		}else
			last_state = 9;
		break;
	case 9: // 解析好了，暂时没什么更新的。就是要等等，以后重连接哈
		// 恢复 modules 哈
		last_state = 2;
		return -1;
		break;
	case 10:// 解析出来，需要更新一些东西哦
		last_state = 0;
		pid = fork();
		if(pid ==0)
			execl("/bin/sh","sh","/tmp/monitor_update",NULL);
		return -1;
	}
	return 0;
}


