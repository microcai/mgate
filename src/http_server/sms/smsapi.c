/*
 * smsapi.c
 *
 *  Created on: 2010-10-12
 *      Author: cai
 */
/*
 * at_modem.c
 *
 *  Created on: 2010-10-12
 *      Author: cai
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
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

static GIOChannel* sendqueue;

gboolean sms_init()
{
	//act as barrier

	GIOChannel * modem=modem_open();

	if(!modem)
		return FALSE;
	int fds[2];

	//建立通信管道
	socketpair(AF_UNIX,SOCK_DGRAM,0,fds);
	fcntl(fds[0],F_SETFD,fcntl(fds[0],F_GETFD)|FD_CLOEXEC);
	fcntl(fds[1],F_SETFD,fcntl(fds[1],F_GETFD)|FD_CLOEXEC);


	fcntl(fds[0],F_SETFL,fcntl(fds[0],F_GETFL)|O_NONBLOCK);
	fcntl(fds[1],F_SETFL,fcntl(fds[1],F_GETFL)|O_NONBLOCK);

	sendqueue = g_io_channel_unix_new(fds[0]);

	thread_param * param  = g_new(thread_param,1);

	param->modem = modem;
	param->queue = g_io_channel_unix_new(fds[1]);

	g_io_channel_set_encoding(sendqueue,NULL,0);
	g_io_channel_set_encoding((GIOChannel*)param->queue,NULL,0);

	g_io_channel_set_buffered(sendqueue,0);
	g_io_channel_set_buffered((GIOChannel*)param->queue,0);

	//开始发送线程
	g_thread_create(sms_send_thread,param,FALSE,0);

	return TRUE;
}


gboolean sms_sendmessage(const gchar * phone,const char * message)
{
	gchar * normalizedphone;
	//首先，规格化电话号码
	if(g_strncasecmp(phone,"+86",3)==0)
	{
		normalizedphone = g_strdup(phone+1);
	}else if(g_strncasecmp(phone,"86",2)==0)
	{
		normalizedphone = g_strdup(phone);
	}else if(g_utf8_strlen(phone,16)==11)
	{
		normalizedphone = g_strdup_printf("86%s",phone);
	}

	//消息不能大于 160 个字符.

	gsize len = g_utf8_strlen(message,300);

	gchar * strSmsc = g_key_file_get_string(gkeyfile,"sms","smsc",NULL);

	//不能超过70个字
	if(len < 70 && strSmsc )
	{
		//进行编码
		SM_PARAM * sm = g_new(SM_PARAM,1);

		strcpy(sm->SCA, strSmsc);
		strcpy(sm->TPA, normalizedphone);
		strcpy(sm->TP_UD, message);

		sm->TP_PID = 0;
		sm->TP_DCS = GSM_UCS2;

		sms_item * item = g_malloc0(sizeof(sms_item)+1000);

		char * pDst = item->pdudata;

		item->pdulen = gsmEncodePdu(sm,pDst);

		//挂入发送列队
		g_io_channel_write_chars(sendqueue,(gchar*)&item,sizeof(item),NULL,NULL);
	}
	g_free(strSmsc);
	g_free(normalizedphone);
	return FALSE;
}
