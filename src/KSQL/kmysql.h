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

#ifdef HAVE_MYSQL_H

int InitMySQL(const std::string & passwd, const std::string & user,
		const std::string & database, const std::string & host);
int InitMysqlDB();

bool is_mysqlserver_gone();

int kmysql_run_query(const char *p);
KSQL_RES* kmysql_query_and_use_result(const char* query);
KSQL_ROW kmysql_fetch_row(KSQL_RES*res);
void kmysql_free_result(KSQL_RES* res);

void kmysql_close();

#else

int InitMysqlDB(){};
bool is_mysqlserver_gone(){};
int kmysql_run_query(const char *p){};
void kmysql_close(){};
void kmysql_free_result(KSQL_RES* res){};
KSQL_RES* kmysql_query_and_use_result(const char* query){};
KSQL_ROW kmysql_fetch_row(KSQL_RES*res){};
int InitMySQL(const std::string & passwd, const std::string & user,
		const std::string & database, const std::string & host){};
#endif

#endif	/* _KMYSQL_H */

