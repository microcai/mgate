

/*
 * 	mysql.cpp
 *
 * 	Copyright 2009 microcai(microcai@sina.com)
 *
 *	microcai's Wrapper for MySQL client lib
 *
 */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <iostream>
#include <queue>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <fcntl.h>
#include <syscall.h>
#include <pthread.h>
#include <syslog.h>
#include <sys/poll.h>

#include <netinet/in.h>
#include <netinet/ip.h>
#include <net/ethernet.h>
#ifdef HAVE_MYSQL
#include <mysql/mysql.h>
#include <mysql/errmsg.h>
#endif
#ifdef WITH_SQLITE3
#include <sqlite3.h>
#endif

#define		__KLIBSQL_USEINTERNALLY
#include "libdreamtop.h"
#include "kmysql.h"

#define __PACKED__

KSQL_RES* ksql_query_and_use_result(const char* query);

static pthread_mutex_t sql_mutex;

static MYSQL mysql;

static const char * create_sql[] =
{

};

int InitMySQL(const std::string & passwd, const std::string & user,
		const std::string & database, const std::string & host)
{
    pthread_mutexattr_t mutex_attr;

 	mysql_init(&mysql);

 	pthread_mutex_destroy(&sql_mutex);

 	pthread_mutexattr_init(&mutex_attr);
 	pthread_mutexattr_settype(&mutex_attr,PTHREAD_MUTEX_RECURSIVE);

 	pthread_mutex_init(&sql_mutex,&mutex_attr);
 	pthread_mutexattr_destroy(&mutex_attr);

 	pthread_mutex_lock(&sql_mutex);

    if(!mysql_real_connect(&mysql,host.c_str(),user.c_str(),passwd.c_str(),database.c_str(), 0, 0, 0))
    {

        if(mysql_errno(&mysql)==1049)
        {
        	syslog(LOG_ERR,"database %s does not exsit, create it.",database.c_str());

        	// 在这里重新建立数据库
        	if (mysql_real_connect(&mysql, host.c_str(), user.c_str(),
					passwd.c_str(), NULL, 0, 0, 0))
			{
        		CString sql;
        		sql.Format("create database %s",database.c_str());
        		mysql_query(&mysql,sql);
        		mysql_select_db(&mysql,database.c_str());
			}
			else goto ERR;
		}
		else
		{
			std::cerr << "Error connecting to database:" << mysql_error(&mysql)
				<< std::endl;
			ERR: mysql_close(&mysql);
			pthread_mutex_unlock(&sql_mutex);
			return mysql_errno(&mysql);
		}
    }
    mysql_set_character_set(&mysql,"utf8");
    pthread_mutex_unlock(&sql_mutex);

    syslog(LOG_NOTICE,"连接到数据库 OK\n");
    return 0;
}

void kmysql_close()
{
	mysql_close(&mysql);
}

bool is_mysqlserver_gone()
{
	char SQL_NULL[] = "SELECT t.`nIndex` FROM t_sysparam t LIMIT 0,0";
	pthread_mutex_lock(&sql_mutex);
	if (mysql_query(&mysql, SQL_NULL))
	{
		pthread_mutex_unlock(&sql_mutex);
		return true;
	}
	else
	{
		int ret = mysql_field_count(&mysql);
		if (ret)
		{
			MYSQL_RES* res = mysql_use_result(&mysql);
			if (res)
				mysql_free_result(res);
		}
		pthread_mutex_unlock(&sql_mutex);
	}
	return false;
}

int kmysql_run_query(const char *p)
{
	pthread_mutex_lock(&sql_mutex);
	int ret = mysql_query(&mysql, p);
	pthread_mutex_unlock(&sql_mutex);
	//尽量减少加锁的时间，增加并行。
	if (ret)
		syslog(LOG_ERR, "err make query  %s\n", mysql_error(&mysql));
	return ret;
}

KSQL_ROW kmysql_fetch_row(KSQL_RES*res)
{
	if(res)
		return mysql_fetch_row((MYSQL_RES*)res);
	else
		return NULL;
}

void kmysql_free_result(KSQL_RES* res)
{
	if (res)
	{
		mysql_free_result((MYSQL_RES*) res);
	}
}

KSQL_RES* kmysql_query_and_use_result(const char* query)
{
	MYSQL_RES * res;
	pthread_mutex_lock(&sql_mutex);

	if (mysql_query(&mysql, query))
	{
		pthread_mutex_unlock(&sql_mutex);
		syslog(LOG_ERR, "Err make query  %s\n", mysql_error(&mysql));
		if (mysql_errno(&mysql) == CR_SERVER_GONE_ERROR)
		{
			close(open("/tmp/monitor.socket", O_RDWR));
		}
		return NULL;
	}
	res = mysql_store_result(&mysql);

	pthread_mutex_unlock(&sql_mutex);

	return (KSQL_RES*) res;
}

int InitMysqlDB()
{
	for (int i = 0; i < (int) (sizeof(create_sql) / sizeof(char*)); ++i)
		mysql_query(&mysql, create_sql[i]);
	return 0;
}

