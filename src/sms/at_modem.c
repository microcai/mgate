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

#include <termios.h>
#include "at_modem.h"

//打开串口并进行适当的波特率设置，返回文件描述符
GIOChannel* modem_open()
{
	int i,comfd;

	GIOChannel * com_io_channel = NULL;

	struct termios tcattr[1];

	struct bitrate{
		int	tcsrateflag;
		int	rate;
	}bitrates[]={
		{50,0000001},
		{75,0000002},
		{110,	0000003},
		{134,	0000004},
		{150,	0000005},
		{200,	0000006},
		{300,	0000007},
		{600,	0000010},
		{1200,	0000011},
		{1800,	0000012},
		{2400,	0000013},
		{4800,	0000014},
		{9600,	0000015},
		{19200,	0000016},
		{38400,	0000017},
		{57600,   0010001},
		{115200,  0010002},
		{230400,  0010003},
		{460800,  0010004},
		{500000,  0010005},
		{576000,  0010006},
		{921600,  0010007},
		{1000000, 0010010},
		{1152000, 0010011},
		{1500000, 0010012},
		{2000000, 0010013},
		{2500000, 0010014},
		{3000000, 0010015},
		{3500000, 0010016},
		{4000000, 0010017},
	};

	speed_t	tcsrate;

	g_return_val_if_fail(gkeyfile,NULL);

	gint 	rate = g_key_file_get_integer(gkeyfile,"sms","rate",NULL);
	gchar * comport = g_key_file_get_string(gkeyfile,"sms","com",NULL);

	for(i = G_N_ELEMENTS(bitrates) -1 ; i >=0 ;i--)
	{
		if (bitrates[i].rate == rate)
		{
			tcsrate = bitrates[i].tcsrateflag;
			break;
		}
	}

	comfd = g_open(comport,O_NOCTTY | O_RDWR | O_SYNC | O_CLOEXEC);

	if(comfd > 0)
	{
		tcgetattr(comfd,tcattr);
		cfsetspeed(tcattr,tcsrate);

		tcattr->c_iflag &= ~(IGNBRK | BRKINT | PARMRK | INLCR | IGNCR | ICRNL| IXON | IXOFF | IXANY);
		tcattr->c_oflag &= ~OPOST;
		tcattr->c_lflag &= ~(ECHO | ECHONL | ECHOE | ICANON | ISIG | IEXTEN);
		tcattr->c_cflag &= ~(CRTSCTS | CSTOPB | PARENB | INPCK | ISTRIP | CSIZE);
		tcattr->c_cflag |= CREAD | CLOCAL | CS8;
		tcattr->c_cc[VMIN] = 1;
		tcattr->c_cc[VTIME] = 0;
		tcflush(comfd, TCIOFLUSH);
		if(tcsetattr(comfd, TCSANOW, tcattr) < 0 )
		{
			close(comfd);
		}
		else
		{
			com_io_channel = g_io_channel_unix_new(comfd);
			g_io_channel_set_encoding(com_io_channel,NULL,NULL);
		}
	}
	return com_io_channel;
}
