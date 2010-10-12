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

typedef struct _sms_item{
	int pdulen;
	char pdudata[];
}sms_item;

static gboolean modem_readed(GIOChannel * modem,GIOCondition cond,gpointer user_data)
{
	char buf[2000];
	gsize readed;
	g_io_channel_read(modem,buf,sizeof(buf),&readed);
	return TRUE;
}

static gpointer sms_send_thread(gpointer data)
{


}

static GIOChannel * modem;
static GAsyncQueue	* sendqueue;

gboolean sms_init()
{
	//act as barrier
	g_atomic_pointer_set(&modem,modem_open());

	if(!modem)
		return FALSE;
	//开始读取
	g_io_add_watch(modem,G_IO_IN,modem_readed,NULL);

	//建立异步列队
	//act as barrier
	g_atomic_pointer_set(&sendqueue,g_async_queue_new_full(g_free));

	//开始发送线程
	g_thread_create(sms_send_thread,modem,FALSE,0);

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

		char * pDst = g_malloc0(1000);

		int pdulen = gsmEncodePdu(sm,pDst);

		//挂入发送列队

		//g_async_queue_push(queue,)

	}
	g_free(strSmsc);
	g_free(normalizedphone);
	return FALSE;
}
