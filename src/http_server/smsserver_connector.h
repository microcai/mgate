/*
 * smsserver_connector.h
 *
 *  Created on: 2010-10-27
 *      Author: cai
 */

#ifndef SMSSERVER_CONNECTOR_H_
#define SMSSERVER_CONNECTOR_H_

typedef struct smsserver_result smsserver_result;

void smsserver_pinger_start();
gboolean smsserver_is_online();

#endif /* SMSSERVER_CONNECTOR_H_ */
