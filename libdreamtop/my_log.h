/*
 * my_log.h
 *
 *  Created on: 2009-4-17
 *      Author: cai
 */

#ifndef MY_LOG_H_
#define MY_LOG_H_
#include <string>

enum LOG_PRINT_LEVEL{
	L_OUTPUT,
	L_FAITAL,
	L_ERROR,
	L_WARNING,
	L_NOTICE,
	L_DEBUG_OUTPUT_MORE,
	L_DEBUG_OUTPUT
};
void log_puts(enum LOG_PRINT_LEVEL level,const std::string );
void log_printf(enum LOG_PRINT_LEVEL level,const char* fmt,...);


#endif /* MY_LOG_H_ */
