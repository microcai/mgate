/*
 * smsthread.c
 *
 *  Created on: 2010-10-13
 *      Author: cai
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <sys/poll.h>
#include <signal.h>
#include <errno.h>
#include <memory.h>
#include <stdlib.h>
#include <stdio.h>
#include <glib.h>
#include <gio/gio.h>
#include <glib/gstdio.h>

#include "i18n.h"
#include "global.h"
#include "at_modem.h"
#include "sms.h"
#include "smsapi.h"
#include "smsthread.h"

static GIOChannel * modem;

static sms_item * getoneitem(GIOChannel * queue)
{
	sms_item * item;
	g_io_channel_read_chars(queue,(gchar*)&item,sizeof(item),NULL,NULL);
	return item;
}

//1 for modem
//2 for sendqueue
//0 for timeout
static int selectfd( GIOChannel * modem, GIOChannel * sendqueue)
{
	struct pollfd fd[2];

	fd[0].fd = g_io_channel_unix_get_fd(modem);
	fd[1].fd = g_io_channel_unix_get_fd(sendqueue);

	fd[0].events = fd[1].events = POLL_IN;


	poll(fd,2,-1);

	if(fd[0].revents & POLL_IN)
		return 1;
	if(fd[1].revents & POLL_IN)
		return 2;
	return 0;
}


gpointer sms_send_thread(gpointer data)
{
	GIOStatus readstatus;
	gsize	written;
	sms_item * item;
	char ans[129];		// 应答串
	gsize ans_len;		// 串口收到的数据长度

	ans[128]=0;

	gchar * ATcommands[]={
			"AT+CGSN\r",
			"AT+CMGF=0\r"
	};

	modem =(GIOChannel*) ((thread_param*)data)->modem;
	GIOChannel	* sendqueue = (GIOChannel*) ((thread_param*)data)->queue;

	g_free(data);

	struct pollfd pfd[1];

	pfd[0].events = POLLIN;
	pfd[0].fd = g_io_channel_unix_get_fd(modem);

	g_io_channel_write_chars(modem,ATcommands[0],strlen(ATcommands[0]),&written,NULL);

	g_usleep(250);

	g_io_channel_write_chars(modem,"AT+CMGF=0\r",sizeof("AT+CMGF=0\r"),&written,NULL);

	if(written != sizeof("AT+CMGF=0\r"))
		return FALSE;

	if (poll(pfd, 1, 300) < 1)
	{
		//me 确信猫有问题
		g_warn_if_reached();
		return FALSE;
	}

	//丢弃返回结果吧
	g_io_channel_read_chars(modem,ans,sizeof(ans),NULL,NULL);

	while(TRUE)
	{
		//选择 COM 口数据还是 线程发来的数据
		switch (selectfd(modem, sendqueue))
		{
			case 1: //没发送命令就有数据过来，一定是有短信来了
			{
				//TODO:读取短信
				g_io_channel_read_chars(modem,ans,sizeof(ans),&ans_len,NULL);

			}
			break;
			case 2:
			{
				ans[128]=0;

				item = getoneitem(sendqueue);
				gchar *cmd;		// 命令串
				unsigned char nSmscLength;	// SMSC串长度

				strcat(item->pdudata, "\x01a");		// 以Ctrl-Z结束

				gsmString2Bytes(item->pdudata, &nSmscLength, 2);	// 取PDU串中的SMSC信息长度
				nSmscLength++;		// 加上长度字节本身

				// 命令中的长度，不包括SMSC信息长度，以数据字节计
				cmd = g_strdup_printf("AT+CMGS=%d\r", item->pdulen / 2 - nSmscLength);// 生成命令

				// 先输出命令串
				g_io_channel_write(modem,cmd,strlen(cmd),&written);
				g_free(cmd);

				pfd[0].events = POLLIN;
				//等待读取ready , 使用 poll 比较好吧, 最多等一秒
				if (poll(pfd, 1, 1000) < 1)
				{
					//接受有错误吧，呵呵
					g_warning("读取modem发生错误");
					g_free(item);
					//出现错误
					break;
				}

				readstatus = g_io_channel_read_chars(modem,ans,sizeof(ans)-1,&ans_len,NULL);

				// 根据能否找到"ERROR "决定成功与否
				if( readstatus!=G_IO_STATUS_NORMAL ||  strstr(ans,"ERROR"))
				{
					g_free(item);
					//出现错误
					break;
				}
				//开始发送编码
				g_io_channel_write_chars(modem,item->pdudata,strlen(item->pdudata),&written,NULL);
				g_usleep(4250);
				g_io_channel_read_chars(modem,ans,sizeof(ans),&ans_len,NULL);
				g_free(item);
			}
		}
	}
	g_io_channel_shutdown(sendqueue,TRUE,0);
	g_io_channel_shutdown(modem,TRUE,0);
	return NULL;
}
