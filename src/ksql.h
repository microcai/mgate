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
GType	ksql_get_backend();

GSQLResult * ksql_query(const gchar * stm);
GSQLResult * ksql_query_free_str(gchar * stm);
GSQLResult * ksql_vquery(const gchar * stmformat , ...);


#endif /* KSQL_H_ */
