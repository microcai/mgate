/*
 * libmicrocai-queue.h
 *
 * Copyright 2009 microcai
 *
 * this a header file for libmicrocai's queue
 *
 */

#ifndef LIBMICROCAI_QUEUE_H_
#define LIBMICROCAI_QUEUE_H_

namespace microcai
{


/******************************************************
 * type defines for a queue
 ******************************************************/
struct List_head
{
	struct List_head*prev, *next;
};
struct List_slot
{
	struct List_head*head, *tail;
};
#ifdef	LIB_QUEUE_INLINE
#define __LIB_QUEUE_INLINE__ __inline
#include "libqueue.cpp"
#else
#define __LIB_QUEUE_INLINE__
/******************************************************
 * functions
 ******************************************************/
void Append(struct microcai::List_head*item,struct microcai::List_slot*slot);
struct microcai::List_head* ReplaceHead(struct microcai::List_head*item, struct microcai::List_slot*slot);
struct microcai::List_head* ReplaceTail(struct microcai::List_head*item,struct microcai::List_slot*slot);
struct microcai::List_head* EraseHead(struct microcai::List_slot*slot);
struct microcai::List_head* EraseTail(struct microcai::List_slot*slot);
#endif

}
#endif /* LIBMICROCAIQUEUE_H_ */
