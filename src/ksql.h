/*
 * ksql.h
 *
 *  Created on: 2010-4-11
 *      Author: cai
 */

#ifndef KSQL_H_
#define KSQL_H_

#include <glib-object.h>
#include "gsqlresult.h"

void	ksql_init(gboolean createdb) ;

GSQLResult * ksql_query(const gchar * stm);
GSQLResult * ksql_query_free_str(gchar * stm);
GSQLResult * ksql_vquery(const gchar * stmformat , ...);
void ksql_query_async(const gchar * stm);
void ksql_query_async_free_str(const gchar * stm);
void ksql_vquery_async(const gchar * stmformat , ...);

#endif /* KSQL_H_ */
