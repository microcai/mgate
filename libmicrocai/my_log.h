/*
 * my_log.h
 *
 *  Created on: 2009-4-17
 *      Author: cai
 */

#ifndef MY_LOG_H_
#define MY_LOG_H_
#include <string>

extern  void lprint(const std::string & logStr);
extern  void lprint(std::string & logStr);
//#define lprint(logStr) lprint((const std::string)logStr)


#endif /* MY_LOG_H_ */
