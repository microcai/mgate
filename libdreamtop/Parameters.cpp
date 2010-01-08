/*
 * Parameters.cpp
 *
 *  Created on: 2009-5-16
 *      Author: cai
 */
#include <iostream>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <dirent.h>
#include <errno.h>

#include "libmicrocai.h"

static inline int stob(const std::string & s)
{
    if (s.compare("yes"))
        return false;
    else
        return true;
}

void ParseParameters(int * argc, char ** argv[], struct parameter_tags p_[])
{
	long i = 0;

	struct parameter_tags *p;
	while (i < *argc)
	{
		p = p_;
		while (p->parameter)
		{
			if (!strncmp((*argv)[i], p->prefix, p->prefix_len))
			{
				switch (p->type)
				{
				case parameter_type::STUB:
					break;
				case parameter_type::BOOL_long:
				case parameter_type::BOOL_both:
					if ((*argv)[i][p->prefix_len] == '=')
					{
						*(int*) p->parameter = stob(std::string((*argv)[i]
								+ p->prefix_len + 1));
						(*argv)[i] = 0;
						break;
					}
					else if ((*argv)[i][p->prefix_len] == 0)
					{
						if ((*argv)[i + 1][0] != '-')
						{
							(*argv)[i] = 0;
							*(int*) p->parameter = stob((*argv)[++i]);
							(*argv)[i] = 0;
						}
						else
						{
							*(int*) p->parameter = true;
							(*argv)[i] = 0;
						}
						break;
					}
					if (p->type == parameter_type::BOOL_long)
						break;
				case parameter_type::BOOL_short:
					*(int*) p->parameter = true;
					if ((*argv)[i][1] != '-')
					{
						strcat((*argv)[i] + 1, (*argv)[i] + 2);
						--i;
					}else (*argv)[i]=0;
					break;
				case parameter_type::INTEGER:
					if ((*argv)[i][p->prefix_len] == 0)
					{
						(*argv)[i]=0;
						++i;
						if (p->parameter_len == sizeof(long))
							*(long*) p->parameter = atol((*argv)[i]);
						else if (p->parameter_len == sizeof(long))
							*(int*) p->parameter = atoi((*argv)[i]);
						else
							throw std::exception();
					}
					else if((*argv)[i][p->prefix_len] == '=')
					{
						if (p->parameter_len == sizeof(long))
							*(long*) p->parameter = atol((*argv)[i]+p->prefix_len+1);
						else if (p->parameter_len == sizeof(int))
							*(int*) p->parameter = atoi((*argv)[i]+p->prefix_len+1);
						else
							throw std::exception();
					}
					(*argv)[i]=0;
					break;
				case parameter_type::STRING:
					if ((*argv)[i][p->prefix_len] == 0)
					{
						(*argv)[i++] = 0;
						strncpy((char*) p->parameter, (*argv)[i],
								p->parameter_len);
					}
					else if ((*argv)[i][p->prefix_len] == '=')
					{
						strncpy((char*) p->parameter, (*argv)[i]
								+ p->prefix_len + 1, p->parameter_len);
					}
					(*argv)[i] = 0;
					break;
				case parameter_type::FUNCTION:
					((void(*)(struct parameter_tags[], const int)) ((void*) p->parameter))
					(p_, p->parameter_len);
				default:
					throw std::exception();
				}
				break;
			}
			p++;
		}
		if( p->parameter == NULL  )
		{
			static char __HELP__[] = "--help";
			if (strncmp((*argv)[i], __HELP__, sizeof(__HELP__))==0)
			{
				p = p_;
				std::cout << "8888888888888888888888888888888888\n";
				while (p->parameter)
				{
					if (p->discribe)
						std::cout << p->discribe << std::endl;
					p++;
				}
				std::cout << "8888888888888888888888888888888888\n";
				pthread_exit(0);
				return;
			}
		}
		++i;
	}
	//删除已经分析的参数
	long a;
	a=i=0;
	while(i < *argc)
	{
		if(  (*argv)[i] )
			(*argv)[a++] =(*argv)[i];
		++i;
	}
	*argc = a;
}
std::string GetToken(const char* strings,const char * key,const char*default_val)
{
	char const * p = strings;
	long l = strlen(key);
	if(!strings)return std::string(default_val);
	std::string ret;
	while( *p)
	{
		if(strncmp(p,key,l)==0)
		{
			while(*p==' ') p++;
			p+=l+1;
			char const * pp = p;
			while(*pp=='=') pp++;
			while(*pp==' ') pp++;

			while(*pp && *pp!='\n' && *pp!='\r' && *pp!=' ')
			{
				ret+= *pp;
				pp++;
			}
			return ret;
		}
		p++;
	}
	if(!ret.size())
		ret=default_val;
	return ret;
}
