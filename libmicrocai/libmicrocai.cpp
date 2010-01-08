/*
 *      libmicrocai.c
 *
 *      Copyright 2009 Unknown <king@tao.kingstone>
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
#include <iostream>
#include <string>
#include <exception>
#include <vector>
#include <map>
#include <iterator>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/eventfd.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <net/ethernet.h>
#include <net/if_arp.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <dlfcn.h>
#include <dirent.h>
#include <errno.h>
#include <pthread.h>

#include "libmicrocai.h"
#include "my_log.h"


u_int16_t checksum(u_int16_t *buffer, int size)
{
    uint32_t cksum = 0;

    while (size > 1)
    {
        cksum += *buffer++;
        size -= sizeof ( unsigned short);
    }

    if (size)
        cksum += *buffer;

    cksum = (cksum >> 16) + (cksum & 0xffff);

    cksum += (cksum >> 16);

    return (uint16_t) (~cksum);
}

static struct handler * head = 0, *tail = 0;
void* register_protocol_handler(PROTOCOL_HANDLER handler, int port, int IPPROTOCOL_TYPE)
{
    struct handler *p = new struct handler();
    p->magic = 'M';
    p->port = port;
    p->protocol_type = IPPROTOCOL_TYPE;
    p->handler = handler;
    if (tail) {
        tail->next = p;
        p->pre = tail;
        tail = p;
    } else {
        tail = head = p;
        head->pre = 0;
        tail->next = 0;
    }
    return (void *) p;
}

PROTOCOL_HANDLER* get_registerd_handler(int port, int IPPROTOCOL_TYPE)
{
    PROTOCOL_HANDLER*ret = 0;
    int i = 0;
    struct handler* p = head;
    ret = (PROTOCOL_HANDLER*)malloc(4096);
    memset(ret, 0, 4096);
    while (p)
    {
        if (p->magic != 'M')break;
        if ((p->port == port || p->port == 0) && p->protocol_type == IPPROTOCOL_TYPE)
            ret[i++] = p->handler;
        p = p->next;
    }
    if (i)return ret;
    free(ret);
    return 0;
}

int un_register_protocol_handler(void*p)
{
    if (p)
    {
        if (((struct handler *) p)->magic != 'M')
            return -1;

        if (((struct handler *) p)->pre)
            ((struct handler *) p)->pre->next = ((struct handler *) p)->next;
        else
            head = ((struct handler *) p)->next;
        if (((struct handler *) p)->next)
            ((struct handler *) p)->next->pre = ((struct handler *) p)->pre;
        else
            tail = ((struct handler *) p)->pre;
        delete (struct handler*)p;
        return 0;
    } else
        return -1;
}
static std::map<std::string,void*> loaded;
static const std::string thisfilename("libmicrocai.so");
static int load_module_recursive(std::string & libname,struct so_data *_so_data)
{
	char * depmod ;
    char * module_name;
    pModuleInitFunc ModuleInitFunc;
    struct so_data init_data;
    std::map<std::string,void*>::iterator it;
    it = loaded.find(libname);
    if( it != loaded.end())
     	return 0;

	void * m = dlopen(libname.c_str(),RTLD_NOW);
	if(m==NULL)
	{
       	std::cerr << "Err loading " << libname << "  for "
        	<< dlerror() << std::endl;

        	return 1;
	}
	loaded.insert( std::pair<std::string,void*>(libname,m));  //更新配对
	*(&depmod) = (char*)dlsym(m,"_depmod_");
	if (depmod)
	{
		char* _depmod = new char[strlen(depmod) + 1]();
		strcpy(_depmod, depmod);
		std::string mod;
		char * tok = strtok(_depmod, ":");
		while(tok)
		{
			mod = tok;
			int ret = load_module_recursive(mod,_so_data);
			if (ret)
			{
				loaded.erase(libname);

				delete[] _depmod;
				std::cerr << "**ERR: Cannot load " << libname;
				std::cerr << ": depend mod " << mod << " failed to load";
				std::cerr << std::endl;

				dlclose(m);
				return ret;
			}
			tok = strtok(0, ":");
		};

		delete[] _depmod;

	}

	*(&module_name) = (char*) dlsym(m, "module_name");
	std::cout << "模块名字：";
	if (module_name)
		std::cout << module_name;
	else
		std::cout << "未知";
	std::cout << "\t模块文件: " << libname;
	std::cout << "\t加载基址 " << m << std::endl;
	std::cout << "--------初始化模块--------";
	std::cout.flush();

	init_data = * _so_data;
	init_data.module = m;

	*(&ModuleInitFunc) = (pModuleInitFunc) dlsym(m, "__module_init");

	if (ModuleInitFunc == NULL)
	{
		std::cout << "\r" << std::endl;
		return 0;
	}
	std::cout << std::endl;
	switch (ModuleInitFunc(&init_data))
	{
	case 0:
		std::cout << "--------模块初始化完毕------" << std::endl << std::endl;
	break;
	case -1:
		std::cerr << "--------------------------" << std::endl;
		std::cerr << "初始化过程中发生了不可恢复的错误，退出!" << std::endl;
		return -1;
	break;
	default:
		std::cout << "-------模块初始化失败!------" << std::endl << std::endl;
		dlclose(m);
	}


	return 0;
}

int enum_and_load_modules(const char*path_to_modules,struct so_data * _so_data)
{
    struct dirent * dt = {0};
    static char LD_LIBRARY_PATH[256];
    sprintf(LD_LIBRARY_PATH, "./%s", path_to_modules);
    setenv("LD_LIBRARY_PATH", LD_LIBRARY_PATH, 0);
    DIR *dir = opendir(path_to_modules);
	std::cout << "**********************开始加载扩展模块*******************************" << std::endl;
	int ret;
	while ((dt = readdir(dir)))
	{
		if (dt->d_type == DT_UNKNOWN || dt->d_type == DT_REG)
		{
			if (dt->d_name[0] != '.' && dt->d_name[0] != 'l' && dt->d_name[0] != 'i' && dt->d_name[0] != 'b')
			{
				/*
				 * 必需是我们要加载的模块。lib*.so是动态库，.*是隐藏的也不要加载。毕竟隐藏有
				 * 其自己的意思的.我们有libmicrocai.so libksql.so两个文件其实不是模块的
				 */

				std::string libname(dt->d_name);
				if (libname[libname.length() - 3] == '.'
						&& libname[libname.length() - 2] == 's'
						&& libname[libname.length() - 1] == 'o')
				{

					ret = load_module_recursive(libname, _so_data);
					if (ret < 0)
					{
						closedir(dir);
						return ret;
					}
				}
			}
		}
	}
	closedir(dir);
	std::cout << "**********************模块加载完毕**********************************" << std::endl;

	std::cout << "*********************加载的模块清单**********************************" << std::endl;
//	static std::map<std::string,void*> loaded;
	std::map<std::string,void*>::iterator it;
	it = loaded.begin();
	void *p;

	while (it != loaded.end())
	{
		p = it->second;
		std::cout << "模块文件名:" << it->first << "\t模块名称:";
		std::cout << (char*) dlsym(p, "module_name") << std::endl;
		it++;
	}
	std::cout << "*******************************************************************" << std::endl;
	std::cout.flush();

	return 0;
}


void Wake_all(int __event)
{
	eventfd_t v=1;
	eventfd_write(__event,v);
	pthread_yield();
	eventfd_read(__event,&v);

}


int Wait_For_event(int __event,int timedout)
{
	pollfd f;
	f.fd = __event;
	f.events = POLLIN;
	f.revents = 0;

	int ret;

	for(;;)
	{
		ret = poll(&f,1,timedout);
		switch(ret)
		{
		case -1:
			if(errno != EINTR)
			return ret;
			else continue;
		case 0:
			return 1;
		case 1:
			if (f.revents & POLLIN)
				return 0;
			else
				return -1;
		}
	}
}

bool GetMac(const char *ip, char MAC_ADDR[],char mac_addr[])
{
	//向内核发送发起ARP查询
	int s = socket(PF_INET, SOCK_DGRAM, 0);
	struct arpreq arpr =
	{
	{ 0 } };

	arpr.arp_flags = ATF_MAGIC;
	arpr.arp_pa.sa_family = AF_INET;
	((struct sockaddr_in*) (&(arpr.arp_pa)))->sin_addr.s_addr = inet_addr(ip);

	//遍历所有以太网卡

	int n = 0;
	do
	{
		sprintf(arpr.arp_dev, "eth%d", n);
		n++;
		//通过ioctl获得arp
		int ret = ioctl(s, SIOCGARP, &arpr);
		if (ret == 0)
		{
			close(s);
			//获得MAC地址;
			u_char* d = (u_char*) arpr.arp_ha.sa_data;
			if(mac_addr)
				memcpy(mac_addr,d,6);
			if(MAC_ADDR)
				sprintf(MAC_ADDR, "%02x:%02x:%02x:%02x:%02x:%02x", d[0], d[1], d[2], d[3], d[4], d[5]);
			return true;
		}
		if (errno == ENXIO)
			continue;
		close (s);
		return false;
	} while(errno != ENODEV);
	close(s);
	return false;
}


void run_cmd(const std::string & strcmd )
{
	//在这里我不得不考虑system失败会导致的资源泄漏。
//	pthread_mutex_lock()
	std::cout << "run: "<< strcmd << std::endl;
	std::cout.flush();
	lprint(strcmd);
//	pthread_atfork(0,0,0);
	system(strcmd.c_str());
}
