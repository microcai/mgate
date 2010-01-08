/*
 * File:   kmysql.h
 * Author: king
 *
 * Created on 2009年4月22日, 下午4:32
 */

#ifndef _KMYSQL_H
#define	_KMYSQL_H
#ifndef __cplusplus
#error ("需要C++编译器支持")
#endif
#pragma once

int InitMySQL(const std::string & passwd, const std::string & user,
		const std::string & database, const std::string & host);
int InitMysqlDB();

bool is_mysqlserver_gone();

KSQL_RES* kmysql_query_and_use_result(const char* query);
KSQL_ROW kmysql_fetch_row(KSQL_RES*res);
void kmysql_free_result(KSQL_RES* res);

void kmysql_close();

#endif	/* _KMYSQL_H */

