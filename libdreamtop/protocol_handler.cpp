/*
 * protocol_handler.cpp
 *
 *
 * Copyright 2009 microcai <microcai@sina.com>
 *
 * See COPYING for more details about this software's license
 */
#include <stdlib.h>
#include <pthread.h>
#include "libmicrocai.h"

static struct handler * head = 0, *tail = 0;
static pthread_rwlock_t	rwlock=PTHREAD_RWLOCK_INITIALIZER;

void* register_protocol_handler(PROTOCOL_HANDLER handler, int port, int IPPROTOCOL_TYPE)
{
    struct handler *p = new struct handler();
    p->magic = 'M';
    p->port = port;
    p->protocol_type = IPPROTOCOL_TYPE;
    p->handler = handler;
    pthread_rwlock_wrlock(&rwlock);
    if (tail) {
        tail->next = p;
        p->pre = tail;
        tail = p;
    } else {
        tail = head = p;
        head->pre = 0;
        tail->next = 0;
    }
    pthread_rwlock_unlock(&rwlock);
    return (void *) p;
}

void get_registerd_handler(PROTOCOL_HANDLER * out_hander, int in_count ,int port, int IPPROTOCOL_TYPE)
{
    int i = 0;
    struct handler* p = head;

    memset(out_hander, 0, in_count * sizeof (PROTOCOL_HANDLER));

    if(pthread_rwlock_tryrdlock(&rwlock)!=0)
    {
    	return;
    }

    while (p)
    {
        if (p->magic != 'M')break;
        if ((p->port == port || p->port == 0) && p->protocol_type == IPPROTOCOL_TYPE)
        	out_hander[i++] = p->handler;
        p = p->next;
    }
    pthread_rwlock_unlock(&rwlock);
}

int un_register_protocol_handler(void*p)
{

    if (p)
    {
        if (((struct handler *) p)->magic != 'M')
            return -1;
    	pthread_rwlock_wrlock(&rwlock);
        if (((struct handler *) p)->pre)
            ((struct handler *) p)->pre->next = ((struct handler *) p)->next;
        else
            head = ((struct handler *) p)->next;
        if (((struct handler *) p)->next)
            ((struct handler *) p)->next->pre = ((struct handler *) p)->pre;
        else
            tail = ((struct handler *) p)->pre;
        pthread_rwlock_unlock(&rwlock);
        delete (struct handler*)p;
        return 0;
    } else
        return -1;

}
