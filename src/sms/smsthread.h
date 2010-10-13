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

G_BEGIN_DECLS

gboolean sms_init();

G_END_DECLS
#endif /* SMSTHREAD_H_ */
