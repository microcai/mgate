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
#include <sys/eventfd.h>
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


static void Wait4Client()
{

    int socketfd;

    int err = 1;

    do
    {
        /*
         * 这里使用SOCK_DGRAM来保留消息边界,同时避免建立链接的步骤，我不想
         * 调式有链接的代码了。讨厌。
         */
        socketfd = socket(AF_UNIX, SOCK_DGRAM, 0);
        fcntl(socketfd,F_SETFD,O_CLOEXEC); // 不要被 fork 继承
        if (socketfd <= 0) break;
        sockaddr_un sock={0};
        sock.sun_family = AF_UNIX;
        strncpy(sock.sun_path, socket_file, 106);
        unlink(socket_file);
        setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &err, sizeof (err));
		if (bind(socketfd, (struct sockaddr*) &sock, sizeof(sock)) < 0)
		{
			std::cerr << strerror(errno);
			break;
		}
        err = 0;


    } while (0);
    if (err) return ;

//    std::cout << "开始监听" << std::endl;

	char cmd_line[256];

    for (;;)
    {
		memset(cmd_line,0,161);

		recvfrom(socketfd,cmd_line,160,0,0,0);
		log_puts(L_DEBUG_OUTPUT_MORE,cmd_line);
		Wake_all();
	}

    close(socketfd);
    unlink(socket_file);
//    std::cout << "chid clients_thread exit" << std::endl;
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
	Wait4Client();
#ifdef DEBUG
	sleep(-1);
#endif
	return 0;
}

