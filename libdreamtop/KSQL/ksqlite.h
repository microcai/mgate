/*
 * ksqlite.h
 *
 *  Created on: 2009-10-21
 *      Author: cai
 */

#ifndef KSQLITE_H_
#define KSQLITE_H_

void InitSqlite();
void InitSqliteDB();

int ksqlite_query(const char * q);

KSQL_RES * ksqlite_query_and_use_result(const char* query);

KSQL_ROW ksqlite_fetch_row(KSQL_RES*res);
void ksqlite_free_result(KSQL_RES* res);

#endif /* KSQLITE_H_ */
