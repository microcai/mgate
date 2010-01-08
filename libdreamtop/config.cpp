
#include <string>
#include <map>
#include <stdio.h>
#include <string.h>
#include <sys/syslog.h>

std::map<std::string,std::string>	configlist;

int prase_config(const char * filecontent, size_t lengh )
{
	char	key[1024],val[1024];
	syslog(LOG_INFO,"cache config file");

	const char * currentline = filecontent;
	while (currentline < (filecontent + lengh))
	{
		bzero(key,sizeof(key));
		bzero(val, sizeof(val));
		if (*currentline != '[')
		{
			sscanf(currentline, "%[^=\n]%*[=]%[^\n]\n", key, val);
			if (strlen(key))
			{
				char * p = key + strlen(key) - 1;
				while (*p == ' ')
					*(p--) = '\0';
				p = val;
				while (*p == ' ')
					*(p++) = '\0';
//				printf("%s=%s\n", key, p);
				configlist.insert(std::pair<std::string, std::string>(key, p));
			}
		}
		while (*currentline != '\n' && (currentline < (filecontent + lengh)))
			currentline++;
		currentline++;
	}
	return 0;
}
std::string GetToken(const char * key,const char*default_val)
{
	std::map<std::string,std::string>::iterator it;
	it = configlist.find(std::string(key));
	if(it==configlist.end())
		return std::string(default_val);
	else
		return it->second;
}

