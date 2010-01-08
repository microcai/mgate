/*
 * protocol_handler.cpp
 *
 *
 * Copyright 2009 microcai <microcai@sina.com>
 *
 * See COPYING for more details about this software's license
 */
#include <stdlib.h>
#include "libmicrocai.h"

static struct handler * head = 0, *tail = 0;
void* register_protocol_handler(PROTOCOL_HANDLER handler, int port, int IPPROTOCOL_TYPE)
{
    struct handler *p = new struct handler();
    p->magic = 'M';
    p->port = port;
    p->protocol_type = IPPROTOCOL_TYPE;
    p->handler = handler;
    if (tail) {
        tail->next = p;
        p->pre = tail;
        tail = p;
    } else {
        tail = head = p;
        head->pre = 0;
        tail->next = 0;
    }
    return (void *) p;
}

PROTOCOL_HANDLER* get_registerd_handler(int port, int IPPROTOCOL_TYPE)
{
    PROTOCOL_HANDLER*ret = 0;
    int i = 0;
    struct handler* p = head;
    ret = (PROTOCOL_HANDLER*)malloc(4096);
    memset(ret, 0, 4096);
    while (p)
    {
        if (p->magic != 'M')break;
        if ((p->port == port || p->port == 0) && p->protocol_type == IPPROTOCOL_TYPE)
            ret[i++] = p->handler;
        p = p->next;
    }
    if (i)return ret;
    free(ret);
    return 0;
}

int un_register_protocol_handler(void*p)
{
    if (p)
    {
        if (((struct handler *) p)->magic != 'M')
            return -1;

        if (((struct handler *) p)->pre)
            ((struct handler *) p)->pre->next = ((struct handler *) p)->next;
        else
            head = ((struct handler *) p)->next;
        if (((struct handler *) p)->next)
            ((struct handler *) p)->next->pre = ((struct handler *) p)->pre;
        else
            tail = ((struct handler *) p)->pre;
        delete (struct handler*)p;
        return 0;
    } else
        return -1;
}
