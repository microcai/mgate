/*
 * uthread.c
 *
 *  Created on: 2010-11-12
 *      Author: cai
 */

#include <ucontext.h>
#include <uthread.h>
#include <malloc.h>

typedef struct thread_context{
    ucontext_t	self;
    struct thread_context* next;
    unsigned long stack[0]; //to make it align
}thread_context,*pthread_context;


struct threadlist{
	pthread_context head;
	pthread_context tail;
	int count;
};

static thread_context	first_one;

static uthread_t 	current = (uthread_t) & first_one;

struct threadlist threadlist = { .head = & first_one  , .tail = & first_one, .count = 1 };

// yes, if some thread exit , put it here ,
// the next thread will clean it for you
static pthread_context	clean_up;


static uthread_t uthread_sched_find_next();
static void uthread_startup(pthread_context tc,__uthread_func fn, void * param);

static void threadlist_append(pthread_context tc);
static pthread_context	threadlist_get_head_and_move_to_tail();
static void threadlist_remove(pthread_context tc);
/**
 * Get self? ... maybe a little bit hard, but ... not hard any way
 */
uthread_t uthread_self()
{
	return current;
}

uthread_t uthread_create(__uthread_func fn, size_t stack_size,void * param)
{
	pthread_context tc = malloc(sizeof(thread_context)+stack_size);

	getcontext(&tc->self);

	tc->self.uc_stack.ss_size = stack_size;
	tc->self.uc_stack.ss_sp = tc->stack;

	makecontext(&tc->self,(void (*) (void))uthread_startup,3,tc,fn,param);

	//append to tail
	threadlist_append(tc);
	return (uthread_t)tc;
}

/**
 * find uthid from global data struct, then switch to it
 */
void uthread_switchto(uthread_t uthid)
{
	uthread_t cur;
	cur = uthread_self();

	if(uthid == cur)
		return;

	//起码你可以 return back了，呵呵
	((pthread_context)uthid)->self.uc_link = & ((pthread_context)cur)->self;

	current = uthid;
	swapcontext(&((pthread_context)cur)->self,&((pthread_context)uthid)->self);

	current = cur;

	if(clean_up)
	{
		//从列表中移走
		threadlist_remove(clean_up);
		free(clean_up);
		clean_up = NULL;
	}
}

/**
 * in fact , all schedule happens here
 */
void uthread_yield()
{
	uthread_t next;

	//to find the next runing thread, and do swapcontext
	next = uthread_sched_find_next();
	if(next) //有下一个线程才切换，否则直接返回
	{
		if(next == uthread_self())
			uthread_yield();
		else
			uthread_switchto(next);
	}
}

/**
 * This is the actual schedule function
 */
static uthread_t uthread_sched_find_next()
{
	pthread_context next;

	next = threadlist_get_head_and_move_to_tail();

	return (uthread_t)next;
}

/**
 * This is the real startup function
 */
static void uthread_startup(pthread_context tc,__uthread_func fn, void * param)
{
	//call user function
	fn(param);

	clean_up = tc;

	setcontext(tc->self.uc_link);
}

static void threadlist_append(pthread_context tc)
{
	threadlist.count ++;
	tc->next = threadlist.head;
	threadlist.tail->next = tc;
	threadlist.tail = tc;
}

static pthread_context	threadlist_get_head_and_move_to_tail()
{
	pthread_context org_head;

	org_head = threadlist.head;
	if(threadlist.count ==1)
		return NULL;

	threadlist.head = threadlist.head->next;
	threadlist.tail = threadlist.tail->next;
	return org_head;
}

static void threadlist_remove(pthread_context tc)
{
	pthread_context r = threadlist.head;

	for(;;)
	{
		if(r->next == tc)
		{
			r->next = tc->next;
			return ;
		}
		r = r->next;
	}

	fprintf(stderr,"function should not reach here\n");

	void exit(int);

	exit((int)-1);
}
