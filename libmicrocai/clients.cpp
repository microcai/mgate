

#include <iostream>
#include <string>
#include <pthread.h>
#include <netinet/in.h>

#include "libmicrocai.h"
#include "my_log.h"

struct client_hash_table_struct
{
	struct client_hash_table_struct * next;
	struct Clients_DATA data;
};

struct client_hash_table_struct *client_hash_table[256];

pthread_rwlock_t lock = PTHREAD_RWLOCK_INITIALIZER;

int is_client_online(in_addr_t ip)
{
	pthread_rwlock_rdlock(&lock);
	struct client_hash_table_struct * ptable =
			client_hash_table[((u_char*) &ip)[3]];
	while (ptable)
	{
		if (ptable->data.ip == ip)
		{
			pthread_rwlock_unlock(&lock);
			return true;
		}
		ptable = ptable->next;
	}
	pthread_rwlock_unlock(&lock);
	return false;
}

void set_client_online(in_addr_t ip, struct Clients_DATA* data) throw()
{
	pthread_rwlock_wrlock(&lock);
	struct client_hash_table_struct * ptable =
			client_hash_table[((u_char*) &ip)[3]];

	while (ptable)
	{
		if (ptable->data.ip == ip)
		{
			ptable->data = *data;
			pthread_rwlock_unlock(&lock);
			return;
		}
		ptable = ptable->next;
	}
	if (client_hash_table[((u_char*) &ip)[3]] == NULL)
	{
		ptable = client_hash_table[((u_char*) &ip)[3]]
				= new struct client_hash_table_struct();
		ptable->next = 0;
		if (data)
			ptable->data = *data;

	} else
	{
		while(ptable->next)
			ptable = ptable->next;
		ptable->next = new struct client_hash_table_struct();
		ptable->next = 0;
        if(data)
            ptable->data = *data;
	}
	pthread_rwlock_unlock(&lock);
}

int set_client_offline(in_addr_t ip)
{
	pthread_rwlock_wrlock(&lock);
	struct client_hash_table_struct * ptable, *p =
			client_hash_table[((u_char*) &ip)[3]];

	if (p == NULL)
	{
		pthread_rwlock_unlock(&lock);
		//谁啊，乱调用！记录这一行为！
		lprint(std::string("set offline called befor set online\n"));
		return false;
	}
	if (p->next == NULL)
	{
		if (p->data.ip == ip)
		{
			client_hash_table[((u_char*) &ip)[3]] = 0;
			delete p;
		}
	}
	else
	{
		ptable = p;
		p = p->next;
		while (p->next)
		{
			if (p->data.ip == ip)
			{
				//删除
				ptable->next = p->next;
				delete p;
				break;
			}
			ptable = p;
			p = p->next;
		}
	}
	pthread_rwlock_unlock(&lock);
	return 1;
}

struct Clients_DATA* get_client_data(in_addr_t ip)
{
	pthread_rwlock_rdlock(&lock);
    struct client_hash_table_struct *ptable =
            client_hash_table[((u_char*) & ip)[3]];
    while (ptable)
        if (ptable->data.ip == ip)
        {
        	pthread_rwlock_unlock(&lock);
            return &(ptable->data);
        }

    /*
     * 都没有上线就调用这个？怎么可能呢！
     * 要记录！
     */
    pthread_rwlock_unlock(&lock);
    lprint(std::string("get_client befor online\n"));
    return NULL;
}

