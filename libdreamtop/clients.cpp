/*
 * clients.cpp
 *
 * Copyright 2009 microcai <microcai@sina.com>
 *
 * See COPYING for more details about this software's license
 */

#include <iostream>
#include <string>
#include <hash_map>
#include <map>
#include <pthread.h>
#include <netinet/in.h>
#include <stdio.h>

#include "libmicrocai.h"
using namespace __gnu_cxx;

struct _HashFn
{
	size_t operator ()(in_addr_t __x) const
	{
		return __x & 0x000000FF;
	}
	size_t operator ()(in_addr_t __x)
	{
		return __x & 0x000000FF;
	}
};


//unordered_map;

struct MACADDR
{
	u_char madd[6];
};

struct myless: public std::binary_function<MACADDR, MACADDR, bool>
{
	bool operator ()(const MACADDR & x, const MACADDR & y)
	{
		return memcmp(&x, &y, 6) < 0;
	}
};

static std::map<MACADDR, Clients_DATA*, myless> mac_ip_map;
static hash_map<in_addr_t, Clients_DATA*, _HashFn> client_hash_map(256);


pthread_rwlock_t lock =PTHREAD_RWLOCK_INITIALIZER;

static void inet_ntoa(std::string & ret, in_addr_t ip)
{
	char buf[32];
	snprintf(buf,32,"%d.%d.%d.%d", ((u_char*) (&ip))[0],
			((u_char*) (&ip))[1], ((u_char*) (&ip))[2], ((u_char*) (&ip))[3]);
	ret = buf;
}


static void set_client_newip(in_addr_t oldip,in_addr_t newip)
{
	hash_map<in_addr_t, Clients_DATA*, _HashFn>::iterator it;

	it = client_hash_map.find(oldip);

	if (it != client_hash_map.end())
	{
		Clients_DATA * pcd = it->second;
		client_hash_map.erase(it);
		pcd->ip = newip;
		inet_ntoa(pcd->ip_addr,newip);
		client_hash_map.insert(std::pair<in_addr_t,Clients_DATA*>(newip,pcd));
	}
}


int is_client_online(u_char mac_addr[6], in_addr_t ip)
{
	std::map<MACADDR, Clients_DATA*>::iterator it;
	hash_map<in_addr_t, Clients_DATA*, _HashFn>::iterator hit;

	MACADDR *pMACADDR = (typeof(pMACADDR)) mac_addr;

	pthread_rwlock_rdlock(&lock);

	hit = client_hash_map.find(ip);

	if (hit != client_hash_map.end())
	{
		pthread_rwlock_unlock(&lock);
		return true;
	}

	//	IP	NOT	FOUND, MAYBE DHCP changed the IP ? or 白名单?
	//	OK, We'll Check it out.
	it = mac_ip_map.find(*pMACADDR);

	if (it == mac_ip_map.end())
	{
		pthread_rwlock_unlock(&lock);

		return false;
	}
	else
	{
		pthread_rwlock_unlock(&lock);
		pthread_rwlock_wrlock(&lock);
		hit = client_hash_map.find(ip);

		if( hit != client_hash_map.end())
		{
			pthread_rwlock_unlock(&lock);
			return true;
		}
		Clients_DATA *pcd;
		pcd = it->second;

		if (pcd->ip)
		{
			nat_disable_ip(pcd->ip_addr.c_str());

			set_client_newip(pcd->ip,ip);

			nat_enbale_ip(pcd->ip_addr.c_str());
			pthread_rwlock_unlock(&lock);
			return true;
		}else //  白名单 ?
		{

			pcd->ip = ip;
			inet_ntoa(pcd->ip_addr,ip);

			CString sql;

			sql.Format("update room_list set IP_ADDR='%s',MAC_ADDR='%s' "
					" where RoomBuild='%s' and RoomFloor='%s' and RoomNum='%s'",
					pcd->ip_addr.c_str(), pcd->mac_addr.c_str(), pcd->Build.c_str(),
					pcd->Floor.c_str(), pcd->RoomNum.c_str());

			ksql_run_query(sql);

			nat_enbale_ip(pcd->ip_addr.c_str());

			client_hash_map.insert(std::pair<in_addr_t,Clients_DATA*>(ip, pcd));

			pthread_rwlock_unlock(&lock);

		}
		return true;
	}
}

void set_client_online(in_addr_t ip, struct Clients_DATA* data)
{


	hash_map<in_addr_t, Clients_DATA*, _HashFn>::iterator hit;

	MACADDR mac ;
	memcpy(mac.madd, data->MAC_ADDR, 6);

	pthread_rwlock_wrlock(&lock);

	if (ip)
	{

		hit = client_hash_map.find(ip);

		if (hit != client_hash_map.end())
		{
			log_printf(L_DEBUG_OUTPUT, "%s already on line\n",
					data->ip_addr.c_str());
		}
		else
		{
			Clients_DATA * pcd = new Clients_DATA(*data);

			client_hash_map.insert(
					std::pair<in_addr_t, Clients_DATA*>(ip, pcd));
			mac_ip_map.insert(std::pair<MACADDR, Clients_DATA*>(mac, pcd));
		}
	}
	else
	{
		Clients_DATA * pcd = new Clients_DATA;
		*pcd = * data;

		mac_ip_map.insert(std::pair<MACADDR, Clients_DATA*>(mac,pcd ));
	}
	pthread_rwlock_unlock(&lock);

}

int set_client_offline(const char* b,const char* f,const char* r)
{
	std::map<MACADDR, Clients_DATA*>::iterator it;
	hash_map<in_addr_t, Clients_DATA*, _HashFn>::iterator hit;
	pthread_rwlock_wrlock(&lock);

	hit = client_hash_map.begin();

	while(hit != client_hash_map.end())
	{
		Clients_DATA * pcd = hit->second;
		if( pcd->Build == b && pcd->Floor == f && pcd->RoomNum == r)
		{
			client_hash_map.erase(hit);
		}
		hit++;
	}

	it = mac_ip_map.begin();

	while(it != mac_ip_map.end())
	{
		Clients_DATA * pcd = it->second;
		if( pcd->Build == b && pcd->Floor == f && pcd->RoomNum == r)
		{
			mac_ip_map.erase(it);
			delete pcd;
			break;
		}
		it++;
	}
	pthread_rwlock_unlock(&lock);
	return 1;
}


int get_client_data(in_addr_t ip,struct Clients_DATA* pcd)
{

	hash_map<in_addr_t, Clients_DATA*, _HashFn>::iterator hit;

	pthread_rwlock_rdlock(&lock);

	hit = client_hash_map.find(ip);
	if (hit != client_hash_map.end())
	{

		*pcd = * hit->second;
		pthread_rwlock_unlock(&lock);
		return 0;
	}
	pthread_rwlock_unlock(&lock);

	/*
	 * 都没有上线就调用这个？怎么可能呢！
	 * 要记录！
	 */

	log_puts(L_ERROR, std::string("get_client befor online\n"));

	return -1;
}

Clients_DATA::Clients_DATA():
	CustomerIDType(""), CustomerName(""), CustomerID(""), RoomNum(""), Floor(""),
			Build(""), mac_addr(""), ip_addr("")
{
	memset(MAC_ADDR,0,6);
	ip= 0;
	onlinetime.tm_year = 0;
}


Clients_DATA::Clients_DATA(in_addr_t _ip):
	CustomerIDType(""), CustomerName(""), CustomerID(""), RoomNum(""), Floor(""),
			Build(""), mac_addr(""), ip_addr("")
{
	onlinetime.tm_year = 0;
	memset(MAC_ADDR,0,6);
	ip = _ip;
}

Clients_DATA::Clients_DATA(const char* cip):
	CustomerIDType(""), CustomerName(""), CustomerID(""), RoomNum(""), Floor(""),
			Build(""), mac_addr(""), ip_addr("")
{
	onlinetime.tm_year = 0;
	memset(MAC_ADDR,0,6);
	ip = inet_addr(cip);
	ip_addr = cip;
}

Clients_DATA::~Clients_DATA()
{

}
