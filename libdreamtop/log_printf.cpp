/*
 * 本文件是用来实现 内存日志 的。
 * 所谓 内存日志，就是将日志记录到内存，以便调查看
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>

#include <sys/signal.h>
#include <signal.h>

#include "libmicrocai.h"

static FILE * log_file;
#ifdef DEBUG
static enum LOG_PRINT_LEVEL debug_level = L_DEBUG_OUTPUT;
#else
static enum LOG_PRINT_LEVEL debug_level=L_ERROR;
#endif

static void handler(int)
{exit(0);}

static void onexit(int, void*)
{
	fwrite("\n--------------------", 1, 20, log_file);
	fwrite("--------------------", 1, 20, log_file);
	fwrite("--------------------", 1, 20, log_file);
	fwrite("----------\n\n\n", 1, 12, log_file);
	fclose(log_file);
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
	on_exit(onexit, 0);
	log_file = fopen("view.log", "a");
}

void set_debug_level(enum LOG_PRINT_LEVEL l)
{
	debug_level = l;
}

void log_printf(enum LOG_PRINT_LEVEL level, const char * fmt, ...)
{
	va_list ap;
	va_start(ap,fmt);
	vfprintf(log_file, fmt, ap);
	fputs("\n", log_file);
	va_end(ap);
	va_start(ap,fmt);
	if (level == L_OUTPUT)
		vprintf(fmt, ap);
	else if (level <= debug_level)
		vfprintf(stderr, fmt, ap);
	va_end(ap);
#ifdef DEBUG
	fflush(log_file);
#endif
}
void log_puts(enum LOG_PRINT_LEVEL level, const std::string str)
{
	fputs(str.c_str(), log_file);
	if (level == L_OUTPUT)
		puts(str.c_str());
	else if (level <= debug_level)
		fprintf(stderr, str.c_str());
#ifdef DEBUG
	fflush(log_file);
#endif
}
