/*
 * uthread.h
 *
 *  Created on: 2010-11-12
 *      Author: cai
 */

#ifndef ___UTHREAD_H_
#define ___UTHREAD_H_

#include <sys/types.h>

typedef unsigned long uthread_t;
typedef void (*__uthread_func)(void * param);

uthread_t uthread_self();
uthread_t uthread_create(__uthread_func,size_t stack_size,void * param);

void uthread_yield();

void uthread_switchto(uthread_t uthid);


#endif /* ___UTHREAD_H_ */
