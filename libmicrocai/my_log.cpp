/*
 * 本文件是用来实现 内存日志 的。
 * 所谓 内存日志，就是将日志记录到内存，以便调查看
 */
#include <string>
#include <list>
#include <iterator>

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/signal.h>
#include <signal.h>
#include <pthread.h>

#include "my_log.h"

std::list<std::string> *list;


static void handler(int){exit(0);}
static void onexit(int, void*)
{

    FILE * f = fopen("view.log","a");

    std::list<std::string>::iterator it;

    it = list->begin();
    size_t i;
    size_t s = list->size();
    for (i = 0; i < s; ++i)
    {
        fwrite(it->c_str(), 1, it->length(),f);
//        fputs(it->c_str(),f);
        it++;
    }

    fwrite("\n--------------------", 1, 20,f);
    fwrite("--------------------", 1, 20,f);
    fwrite("--------------------", 1, 20,f);
    fwrite("----------\n\n\n", 1, 12,f);
    fclose(f);
	//    list->clear();
}
//static void _Befor_fork()
//{
//
//}
//static void _After_Fork()
//{
//
//}
static void __attribute__((constructor)) onload()
{
	struct sigaction action;
	action.sa_flags = SA_NODEFER;
	action.sa_handler = handler;
	sigaction(2, &action, &action);

//	pthread_atfork(_Befor_fork,_After_Fork,0);


    list = new std::list<std::string>;
    on_exit(onexit, 0);
    std::string s("monitor loaded at ");
    time_t t;
    t = time(0);
    s += ctime(&t);
    list->insert(list->end(),s);
}
static pthread_mutex_t lprint_lock=PTHREAD_MUTEX_INITIALIZER;

void lprint( const std::string  &  logStr)
{
	time_t tim;
	tim = time(0);
	std::string p;
	p = ctime(&tim);
	p.erase(p.length()-1,1);

	p+= std::string(": ");
	p += logStr;
	p += "\n";

	pthread_mutex_lock(&lprint_lock);

	if (list->size() > 50)
	{

		FILE * f = fopen("view.log", "a");

		std::list<std::string>::iterator it;

		it = list->begin();
		size_t i;
		size_t s = list->size();
		for (i = 0; i < s; ++i)
		{
			fwrite(it->c_str(), 1, it->length(), f);
			it++;
		}
		fclose(f);
		list->clear();
	}
    list->insert(list->end(),p);
    pthread_mutex_unlock(&lprint_lock);
}

void lprint(std::string & logStr)
{
	lprint((const std::string)logStr);
}
