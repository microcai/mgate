/*
 * smsthread.h
 *
 *  Created on: 2010-10-13
 *      Author: cai
 */

#ifndef SMSTHREAD_H_
#define SMSTHREAD_H_

#include <glib.h>

typedef struct _sms_item{
	int pdulen;
	char pdudata[];
}sms_item;

typedef struct thread_param{
	volatile GIOChannel * modem;
	volatile GIOChannel * queue;
}thread_param;

G_BEGIN_DECLS

gpointer sms_send_thread(gpointer data) G_GNUC_INTERNAL;

G_END_DECLS
#endif /* SMSTHREAD_H_ */
