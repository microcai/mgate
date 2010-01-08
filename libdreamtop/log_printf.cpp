/*
 * 本文件是用来实现 内存日志 的。
 * 所谓 内存日志，就是将日志记录到内存，以便调查看
 */

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>

#include <sys/signal.h>
#include <signal.h>

#include "libmicrocai.h"

#ifdef DEBUG
static enum LOG_PRINT_LEVEL debug_level = L_DEBUG_OUTPUT;
#else
static enum LOG_PRINT_LEVEL debug_level=L_NOTICE;
#endif

void set_debug_level(enum LOG_PRINT_LEVEL l)
{
	debug_level = l;
}

void log_printf(enum LOG_PRINT_LEVEL level, const char * fmt, ...)
{
	va_list ap;
	va_start(ap,fmt);
	va_start(ap,fmt);
	if (level == L_OUTPUT)
		vprintf(fmt, ap);
	else if (level <= debug_level)
		vfprintf(stderr, fmt, ap);
	va_end(ap);
}
void log_puts(enum LOG_PRINT_LEVEL level, const std::string str)
{
	if (level == L_OUTPUT)
		puts(str.c_str());
	else if (level <= debug_level)
		fprintf(stderr, str.c_str());
}
