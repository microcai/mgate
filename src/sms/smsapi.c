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


static gboolean modem_readed(GIOChannel * modem,GIOCondition cond,gpointer user_data)
{
	char buf[2000];
	gsize readed;
	g_io_channel_read(modem,buf,sizeof(buf),&readed);
	return TRUE;
}

static GIOChannel * modem;
gboolean sms_init()
{
	modem = modem_open();

	if(!modem)
		return FALSE;
	//开始读取

	g_io_add_watch(modem,G_IO_IN,modem_readed,NULL);

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

	const gchar * strSmsc = g_key_file_get_string(gkeyfile,"sms","smsc",NULL);

	if(len < 160 && strSmsc )
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

	}
	g_free(strSmsc);
	g_free(normalizedphone);
	return FALSE;
}
