/*
 * at_modem.h
 *
 *  Created on: 2010-10-12
 *      Author: cai
 */

#ifndef AT_MODEM_H_
#define AT_MODEM_H_

#include <gio/gio.h>

//打开串口并进行适当的波特率设置，返回GIOChannel
extern GIOChannel* modem_open();

#endif /* AT_MODEM_H_ */
