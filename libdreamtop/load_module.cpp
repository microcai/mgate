/*
 * load_module.cpp
 *
 * Copyright 2009 microcai <microcai@sina.com>
 *
 * See COPYING for more details about this software's license
 */

#include <iostream>
#include <string>
#include <map>

#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <unistd.h>
#include <dlfcn.h>
#include <dirent.h>
#include <fcntl.h>

#include "libdreamtop.h"

static std::map<std::string,void*> loaded;
static const std::string thisfilename("libmicrocai.so");

static int load_module_recursive(std::string & libname,struct so_data *_so_data,const char *path_to_modules)
{
	char * depmod;
	char * module_name;
	pModuleInitFunc ModuleInitFunc;
	struct so_data init_data;
	std::map<std::string, void*>::iterator it;
	it = loaded.find(libname);
	if (it != loaded.end())
		return 0;

	void * m = dlopen(libname.c_str(), RTLD_LAZY);
	if (m == NULL)
	{
		syslog(LOG_ERR,"Err loading %s for %s",libname.c_str(),dlerror());
		return 1;
	}
	loaded.insert(std::pair<std::string, void*>(libname, m)); //更新配对

	*(&depmod) = (char*) dlsym(m, "_depmod_");
	if (depmod)
	{
		char* _depmod = new char[strlen(depmod) + 1]();
		strcpy(_depmod, depmod);
		std::string mod;
		char * tok = strtok(_depmod, ":");
		while (tok)
		{
			mod = tok;
			int ret = load_module_recursive(mod, _so_data,path_to_modules);
			if (ret)
			{
				loaded.erase(libname);

				delete[] _depmod;
				syslog(LOG_ERR,"**ERR: Cannot load %s",libname.c_str());
				syslog(LOG_ERR,": depend mod %s failed to load",mod.c_str());

				dlclose(m);
				return ret;
			}
			tok = strtok(0, ":");
		}
		delete[] _depmod;
	}

	*(&module_name) = (char*) dlsym(m, "module_name");
	std::cout << "模块名字：";
	if (module_name)
		std::cout << module_name;
	else
		std::cout << "未知";
	std::cout << "\t模块文件: " << libname;
	std::cout << "\t加载基址 " << m << std::endl;
	std::cout << "--------初始化模块--------";
	std::cout.flush();

	init_data = *_so_data;
	init_data.module = m;

	*(&ModuleInitFunc) = (pModuleInitFunc) dlsym(m, "__module_init");

	if (ModuleInitFunc == NULL)
	{
		std::cout << "\r" << std::endl;
		return 0;
	}
	std::cout << std::endl;
	switch (ModuleInitFunc(&init_data))
	{
	case 0:
		std::cout << "--------模块初始化完毕------" << std::endl << std::endl;
		break;
	case -1:
		std::cerr << "--------------------------" << std::endl;
		std::cerr << "初始化过程中发生了不可恢复的错误，退出!" << std::endl;
		return -1;
		break;
	default:
		std::cout << "-------模块初始化失败!------" << std::endl << std::endl;
		dlclose(m);
	}
	return 0;
}

int unload_modules(const char * so_name)
{
	std::map<std::string,void*>::iterator it;
	it = loaded.begin();

	void *p;

	std::string soname;

	while (it != loaded.end())
	{
		p = & it->second;

		soname = it->first;

		if (soname == so_name)
		{
			syslog(LOG_NOTICE, "unload %p %s\n", p, soname.c_str());
			dlclose(p);
			loaded.erase(it);
			return 0;
		}
		it++;
	}
	return 0;
}

void load_modules(const char * so_name,const char*path_to_modules)
{
	so_data sodata;
	std::string soname(so_name);
	if (so_name[0] != '.' && so_name[0] != 'l' && so_name[0] != 'i' && so_name[0] != 'b')
		load_module_recursive( soname, & sodata,path_to_modules);

	std::map<std::string,void*>::iterator it;
	it = loaded.begin();

	while (it != loaded.end())
	{
		void *p = it->second;

		int (*so_can_unload)();

		so_can_unload = (typeof(so_can_unload))dlsym(p, "so_can_unload");

		if( so_can_unload && so_can_unload() == 0 )
		{
			loaded.erase(it);
			it = loaded.begin();
		}
		it++;
	}

}

void reload_modules(const char * so_name,const char*path_to_modules)
{
	if(unload_modules(so_name)==0)
		load_modules(so_name,path_to_modules);

}

int enum_and_load_modules(const char*path_to_modules)
{
	struct so_data _so_data;
 	int ret;
 	struct dirent * dt;
    DIR *dir = opendir(path_to_modules);
	if(!dir)
	{
		syslog(LOG_ERR,"WARNNING: 没有找到扩展模块");
		return -1;
	}

	std::cout << "**********************开始加载扩展模块*******************************" << std::endl;

	while ((dt = readdir(dir)))
	{
		if (dt->d_type == DT_UNKNOWN || dt->d_type == DT_REG)
		{
			if (dt->d_name[0] != '.' && dt->d_name[0] != 'l' && dt->d_name[0] != 'i' && dt->d_name[0] != 'b')
			{
				/*
				 * 必需是我们要加载的模块。lib*.so是动态库，.*是隐藏的也不要加载。毕竟隐藏有
				 * 其自己的意思的.我们有libmicrocai.so libksql.so两个文件其实不是模块的
				 */

				std::string libname(dt->d_name);
				if (libname[libname.length() - 3] == '.'
						&& libname[libname.length() - 2] == 's'
						&& libname[libname.length() - 1] == 'o')
				{

					ret = load_module_recursive(libname, &_so_data,path_to_modules);
					if (ret < 0)
					{
						closedir(dir);
						return ret;
					}
				}
			}
		}
	}
	closedir(dir);
	std::cout << "**********************模块加载完毕**********************************" << std::endl;

	std::cout << "*********************加载的模块清单**********************************" << std::endl;

	std::map<std::string,void*>::iterator it;
	it = loaded.begin();
	void *p;

	while (it != loaded.end())
	{
		p = it->second;

		std::cout << "模块文件名:" << it->first <<  "\t模块名称:";
		std::cout << (char*) dlsym(p, "module_name") << std::endl;
		it++;
	}

	while (it != loaded.end())
	{
		p = it->second;

		int (*so_can_unload)();

		so_can_unload = (typeof(so_can_unload))dlsym(p, "so_can_unload");

		if( so_can_unload && so_can_unload() == 0 )
		{
			loaded.erase(it);
			it = loaded.begin();
		}
		it++;
	}

	std::cout << "*******************************************************************" << std::endl;
	std::cout.flush();

	return 0;
}
