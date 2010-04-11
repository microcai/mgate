/*
 * ksql.h
 *
 *  Created on: 2010-4-11
 *      Author: cai
 */

#ifndef KSQL_H_
#define KSQL_H_

#ifndef HAVE_MYSQL_MYSQL_H

#define  FUNCTION_DEFINE	{};

#else

#define  FUNCTION_DEFINE 	;
#endif

void	ksql_init() FUNCTION_DEFINE
gboolean	ksql_connect_sql() FUNCTION_DEFINE
void ksql_create_db() FUNCTION_DEFINE


#endif /* KSQL_H_ */
