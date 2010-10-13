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
static GAsyncQueue	* sendqueue;

static gpointer sms_send_thread(gpointer data)
{
	gsize	written;
	sms_item * item;

	g_atomic_pointer_set(&sendqueue,data);

	while ((item = g_async_queue_pop(sendqueue)))
	{
		gchar *cmd;		// 命令串
		unsigned char nSmscLength;	// SMSC串长度
		int nLength;		// 串口收到的数据长度

		char ans[128];		// 应答串

		strcat(item->pdudata, "\x01a");		// 以Ctrl-Z结束

		gsmString2Bytes(item->pdudata, &nSmscLength, 2);	// 取PDU串中的SMSC信息长度
		nSmscLength++;		// 加上长度字节本身

		// 命令中的长度，不包括SMSC信息长度，以数据字节计
//		sprintf(cmd, "AT+CMGS=%d\r", nPduLength / 2 - nSmscLength);
		cmd = g_strdup_printf("AT+CMGS=%d\r", item->pdulen / 2 - nSmscLength);// 生成命令

		// 先输出命令串
		g_io_channel_write(modem,cmd,strlen(cmd),&written);


		//开始发送编码


	}

	g_async_queue_unref(sendqueue);
	return NULL;
}

static gboolean modem_readed(GIOChannel * modem,GIOCondition cond,gpointer user_data)
{
	char buf[2000];
	gsize readed;
	g_io_channel_read(modem,buf,sizeof(buf),&readed);
	return TRUE;
}

gboolean sms_init()
{
	//act as barrier
	g_atomic_pointer_set(&modem,modem_open());

	if(!modem)
		return FALSE;
	//开始读取
//	g_io_add_watch(modem,G_IO_IN,modem_readed,NULL);

	//建立异步列队
	sendqueue = g_async_queue_new_full(g_free);

	//开始发送线程
	g_thread_create(sms_send_thread,sendqueue,FALSE,0);

	return TRUE;
}
