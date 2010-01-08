/*
 * clients.cpp
 *
 * Copyright 2009 microcai <microcai@sina.com>
 *
 * See COPYING for more details about this software's license
 */


#include <iostream>
#include <string>
#include <map>
#include <pthread.h>
#include <netinet/in.h>

#include "libmicrocai.h"

struct client_hash_table_struct
{
	struct client_hash_table_struct * next;
	struct Clients_DATA data;
};

struct client_hash_table_struct *client_hash_table[256];

struct MACADDR{
	char	madd[6];
};


struct myless:public std::binary_function <MACADDR, MACADDR, bool >
{
	bool operator ()(const MACADDR & x ,const MACADDR & y)
	{
		return memcmp(&x,&y,6)<=0;
	}
};

static std::map<MACADDR, in_addr_t , myless> mac_ip_map;

pthread_rwlock_t lock = PTHREAD_RWLOCK_INITIALIZER;
pthread_mutex_t locklock = PTHREAD_MUTEX_INITIALIZER;

int is_client_online( char mac_addr[6], in_addr_t ip)
{
	std::map<MACADDR, in_addr_t>::iterator it;

	MACADDR *pMACADDR = (typeof(pMACADDR)) mac_addr;


	pthread_rwlock_rdlock(&lock);
    struct client_hash_table_struct *ptable = client_hash_table[((u_char*) & ip)[3]];

    while(ptable)
    {
    	if(ptable->data.ip == ip)
    	{
    		pthread_rwlock_unlock(&lock);
    		return true;
    	}
    }
    //	IP	NOT	FOUND, MAYBE DHCP changed the IP ?
	//	OK, We'll Check it out.
	it = mac_ip_map.find(*pMACADDR);

	if (it == mac_ip_map.end())
	{
		pthread_rwlock_unlock(&lock);
		return false;
	}
	else
	{
		pthread_mutex_lock(&locklock);
		pthread_rwlock_unlock(&lock);
		pthread_rwlock_wrlock(&lock);
		pthread_mutex_unlock(&locklock);

		Clients_DATA * pcd = get_client_data(it->second);

		set_client_online(ip, pcd);
		set_client_offline(ip);

		pthread_rwlock_unlock(&lock);

		// enable NAT table for the new ip



		return true;
	}
}




void set_client_online(in_addr_t ip, struct Clients_DATA* data) throw()
{
	pthread_mutex_lock(&locklock);
	pthread_rwlock_wrlock(&lock);
	pthread_mutex_unlock(&locklock);
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
	pthread_mutex_lock(&locklock);
	pthread_rwlock_wrlock(&lock);
	pthread_mutex_unlock(&locklock);
	struct client_hash_table_struct * ptable, *p =
			client_hash_table[((u_char*) &ip)[3]];

	if (p == NULL)
	{
		pthread_rwlock_unlock(&lock);
		//谁啊，乱调用！记录这一行为！
		log_puts(L_ERROR,std::string("set offline called befor set online\n"));
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
    log_puts(L_ERROR,std::string("get_client befor online\n"));
#ifdef DEBUG
    {
    	static Clients_DATA FAKE;

    	FAKE.Build = "1";
    	FAKE.CustomerID = "10001101010";
    	FAKE.CustomerIDType = "114";
    	FAKE.CustomerName = "test";
    	FAKE.Floor = "1";
    	strcpy(FAKE.MAC_ADDR,"asdfawe");
    	FAKE.mac_addr = "00-00-00-00-00-00";
    	in_addr l;
    	l.s_addr = ip;
    	FAKE.ip_addr = inet_ntoa(l);
		return &FAKE;
   }
#endif
    return NULL;
}

