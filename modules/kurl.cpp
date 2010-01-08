

#include <iostream>
#include <map>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <net/ethernet.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <vector>

#include "libmicrocai.h"
#include "kmysql.h"

#include <ctime>
std::map<std::string, time_t> url_time_map;

struct HOSTDATA
{
	char strIP[20];
	char strHost[50];
};
struct MAIL_LOGIN_KEY
{
	char user[16];
	char password[16];
	char select[16];
};

// Parameter sep is char set
static size_t GetSepTextByChar(const std::string &strText, const char *sep, std::vector<std::string> &vec)
{
	if (sep == NULL)
		return 0;
	size_t startpos = 0, endpos =0;

	while(startpos != (size_t)-1)
	{
		startpos = strText.find_first_not_of(sep, startpos);
		if (startpos == (size_t)-1)
			break;

		endpos = strText.find_first_of(sep, startpos);

		std::string::size_type end = (endpos == (size_t) -1)?strText.size():endpos;
		std::string temp = strText.substr(startpos, end - startpos);

		if (!temp.empty())
			vec.push_back(temp);
		startpos = endpos;
	}
	return vec.size();
}

static bool ParseTcpPkt(const char* tcpdata, size_t tcpdata_len,
				 std::string& url, std::string& usr, std::string& pwd)
{
	//data begin with "POST"
	if(memcmp(tcpdata,"POST ",5))
	    return false;

	//data end with "\r\n\r\n"
	if(!strstr(tcpdata,"\r\n\r\n"))
	  return false;
	std::string strdata(tcpdata, tcpdata_len);

	size_t pos = std::string::npos;
	if ((pos = strdata.find("\r\n\r\n")) == std::string::npos)
		return false;



	std::string html_header = strdata.substr(0, pos+4);
	std::string html_body = strdata.substr(pos+4, strdata.size() -pos-4);


	//url
	size_t p1 = std::string::npos, p2 = std::string::npos;

	if ((p1 = html_header.find("Host: ")) == std::string::npos ||
		(p2 = html_header.find("\r\n", p1)) == std::string::npos)
		return false;
	p1 += strlen("Host: ");
	url = html_header.substr(p1, p2-p1);

	//user and password
	std::map<std::string, std::string> usr_pwd_map;
	std::vector<std::string> line_vector;
	if (GetSepTextByChar(html_body, "&", line_vector) == 0)
		return false;

	std::vector<std::string>::const_iterator it = line_vector.begin();
	for (;it != line_vector.end(); ++it)
	{
		std::string line(*it);
		size_t sep_pos = std::string::npos;
		if ( (sep_pos = line.find('=')) == std::string::npos)
			continue;

		std::string strkey = line.substr(0, sep_pos);
		std::string strvalue = line.substr(sep_pos +strlen("="), line.size() -sep_pos);
		usr_pwd_map[strkey] = strvalue;
	}

	std::multimap<std::string, std::string> keyword_map;
	keyword_map.insert(std::make_pair("username", "password"));
	keyword_map.insert(std::make_pair("username", "passwd"));
	keyword_map.insert(std::make_pair("pwuser", "pwpwd"));

	std::multimap<std::string, std::string>::iterator it_map = keyword_map.begin();
	for (; it_map != keyword_map.end(); ++it_map)
	{
		if(usr_pwd_map.find(it_map->first) != usr_pwd_map.end() &&
			usr_pwd_map.find(it_map->second) != usr_pwd_map.end())
		{
			usr = usr_pwd_map[it_map->first];
			pwd = usr_pwd_map[it_map->second];
			return true;
		}
	}

	return false;
}

static int RecordPOST(const char *user, const char*pswd, const char*host, u_char*packet,in_addr_t sip,in_addr_t dip)
{
	struct NetAcount na(NetAcountType_POST,packet);// = (struct NetAcount*)malloc(8190);

	struct tcphdr* tcp = (tcphdr*)(packet + 14 + sizeof(iphdr));

	char	strhost[32];
	snprintf(strhost,32,"%d.%d.%d.%d",((u_char*)&dip)[0],((u_char*)&dip)[1],((u_char*)&dip)[2],((u_char*)&dip)[3]);

	na.data = host;
	na.ip = sip;
	na.dstip = dip;

	strcpy(na.strType,"0006");

	char key2[80];
	snprintf(key2,80,"%s:%s",user,pswd);
	na.passwd = key2 ;
	na.host = strhost;
	na.dport = ntohs(tcp->dest);

    RecordAccout(&na);
    log_puts(L_DEBUG_OUTPUT_MORE,user);
    return 1;
}

static int RecordUrl(char *url,u_char*packet,in_addr_t sip,in_addr_t dip)
{
	static pthread_mutex_t lock=PTHREAD_MUTEX_INITIALIZER;
	std::string strUrl(url);
	size_t nstart = std::string::npos;
	size_t count_splash = 0;


	while ((nstart = strUrl.find('/', nstart +1)) != std::string::npos)
		++count_splash;

	if (count_splash >3)
		return 0;

	time_t tmNow = time(NULL);

	pthread_mutex_lock(&lock);
	std::map<std::string, time_t>::iterator it = url_time_map.begin();
	for (; it != url_time_map.end();)
	{
		if (tmNow - it->second > 60*2)
			url_time_map.erase(it++);
		else
			++it;
	}

	if (url_time_map.find(strUrl) != url_time_map.end())
	{
		pthread_mutex_unlock(&lock);
		return 0;
	}

	url_time_map[strUrl] = tmNow;
	pthread_mutex_unlock(&lock);

    log_printf(L_DEBUG_OUTPUT_MORE,"URL is %s\n",url);
    struct NetAcount na(NetAcountType_HTTP,packet);// = (struct NetAcount*)malloc(8190);

	na.data = url;
	na.ip = sip;
	na.dstip = dip;
	strcpy(na.strType,"0001");

	na.dport =  (80);
    RecordAccout(&na);

    return 1;
}

static int GetHttpPost(struct so_data*, u_char*packet)
{
	/**************************************************
	 *IP数据包通常就在以太网数据头的后面。以太网头的大小为14字节*
	 **************************************************/
	struct iphdr * ip_head = (struct iphdr*) (packet + ETH_HLEN);
	/**************************************************
	 *TCP头也就在IP头的后面。IP头的大小为20字节，不过最好从头里读取
	 **************************************************/
	struct tcphdr * tcp_head = (struct tcphdr*) (packet + ETH_HLEN
			+ ip_head->ihl * 4);
	/**************************************************
	 *TCP数据现对于tcp头的偏移由doff给出。这个也是tcp头的大小**
	 **************************************************/
	char* tcpdata = (char*) tcp_head + tcp_head->doff * 4;
	int tcpdatelen = ntohs(ip_head->tot_len) - tcp_head->doff * 4 - ip_head->ihl * 4;
	/*太小的包肯定就不是*/
	if (tcpdatelen < 10)
		return 0; // POST \r\n\r\n 几个字节？

	//add 090901
	std::string url, usr, pwd;
	if( ParseTcpPkt(tcpdata, tcpdatelen, url, usr, pwd) == true)
	{
		return RecordPOST(usr.c_str(), pwd.c_str(), url.c_str(), packet,ip_head->saddr, ip_head->daddr);
	}
	return 1;
	//end 090901

	HOSTDATA* pHostData;
	std::map<in_addr_t, HOSTDATA>::iterator it;
	static std::map<in_addr_t,HOSTDATA> host_list;
	static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
	pthread_mutex_lock(&lock);
	it = host_list.find(ip_head->daddr);
	if (it != host_list.end())
		pHostData = &it->second;
	else
		pHostData = NULL;
	pthread_mutex_unlock(&lock);
	const int nSize = 50;
	char pUser[50] =
	{ 0 };
	char pPassword[50] =
	{ 0 };
	char pHost[50] =
	{ 0 };

	if (!pHostData)
	{
		if (memcmp("POST ", tcpdata, 5))
			return 0;

		char *pPos = NULL;
		pPos = strstr((char *) tcpdata, "Host:");
		if (!pPos) return 0 ;

		pPos = pPos + 6;
		char *pTemp = strstr(pPos, "\r\n");
		if (pTemp)
		{
			if (pTemp - pPos >= nSize)
				return 0;
			if (pTemp - pPos < 4)
				return 0;
			strncpy(pHost, pPos, pTemp - pPos);
			HOSTDATA HostData;
			inet_neta(ip_head->daddr, HostData.strIP, 20);
			strcpy(HostData.strHost, pHost);
			pthread_mutex_lock(&lock);
			if (host_list.size() > 2000)
				host_list.erase(host_list.begin());
			host_list.insert(std::pair<in_addr_t, HOSTDATA>(
					ip_head->daddr, HostData));
			pthread_mutex_unlock(&lock);
		}
	}
	else
	{
		strcpy(pHost, pHostData->strHost);
	}

	MAIL_LOGIN_KEY loginKeys[] =
	{
	{ "username=", "password=", "&" },
	{ "pwuser=", "pwpwd=", "&" },
	{ "user=", "password=", "&" },
	{ "user=", "pwd=", "&" },
	{ "pwuser=", "password=", "&" },
	{ "account=", "pass=", "&" },
	{ "account=", "password=", "&" },
	{ "pwuser=", "pass=", "&" },
	{ {0},{ 0}, {0} } };
	tcpdata = strstr(tcpdata,"\r\n\r\n");

	if(!tcpdata)
		return 0;

	for (int i = 0;; i++)
	{
		MAIL_LOGIN_KEY &key = loginKeys[i];

		if (!key.user)
			break;

		char *pPos = NULL;
		char *pTemp = NULL;

		pPos = strstr((char *) tcpdata, key.user);
		if (pPos)
		{
			if (strlen(pUser) == 0)
			{
				pPos = pPos + strlen(key.user);
				pTemp = strstr(pPos, key.select);
				if (pTemp)
				{
					if (pTemp - pPos >= nSize)
						continue;
					strncpy(pUser, pPos, pTemp - pPos);
					//log_printf(L_DEBUG_OUTPUT,"username: % \n", pUser);
				}
				else
					continue;
			}

			pPos = strstr(tcpdata, key.password);
			if (pPos)
			{
				pPos = pPos + strlen(key.password);
				pTemp = strstr(pPos, key.select);
				if (pTemp)
				{
					if (pTemp - pPos >= nSize)
						continue;
					strncpy(pPassword, pPos, pTemp - pPos);
					return RecordPOST(pUser,pPassword,pHost,packet,ip_head->saddr, ip_head->daddr);
				}
				else
					continue;
			}
		}
	}
	if (strlen(pUser))
		return RecordPOST(pUser, pPassword, pHost, packet,ip_head->saddr, ip_head->daddr);
	return 0;
}

static int url_packet_callback(struct so_data* sodata, u_char * packet)
{
    /**************************************************
     *IP数据包通常就在以太网数据头的后面。以太网头的大小为14字节*
     **************************************************/
    struct iphdr * ip_head = (struct iphdr*) (packet + ETH_HLEN);
    /**************************************************
     *TCP头也就在IP头的后面。IP头的大小为20字节，不过最好从头里读取
     **************************************************/
    struct tcphdr * tcp_head = (struct tcphdr*) (packet + ETH_HLEN + ip_head->ihl * 4);
    /**************************************************
     *TCP数据现对于tcp头的偏移由doff给出。这个也是tcp头的大小**
     **************************************************/
    char* tcpdata = (char*) tcp_head + tcp_head->doff * 4;
   /// if(!tcp_head->psh)return 0;
    int tcpdatelen = ntohs(ip_head->tot_len) - tcp_head->doff * 4 - ip_head->ihl * 4;
    /*太小的包肯定就不是*/
    if (tcpdatelen < 10)return 0; // GET / HTTP/1.1\r\n\r\n 几个字节？

    const int nSize = 4096;
    char *pUrl = new char[nSize];

    memset(pUrl, 0, nSize);
    static char strOldUrl[5][250];
    static int nCurPos = 0;
    if (memcmp("GET ", tcpdata , 4) == 0 || memcmp("POST ", tcpdata, 5) == 0)
    {
        char *pPos = NULL;
        pPos = strstr((char *) tcpdata , "Referer:");

        if (pPos)
        {
            pPos = pPos + 9;
			char *pTemp = strstr(pPos, "\r\n");
			if (pTemp)
			{
				if (pTemp - pPos >= nSize)
					return 0;
				strncpy(pUrl, pPos, pTemp - pPos);
				for (int n = 0; n < 5; n++)
				{
					if (strcmp(strOldUrl[n], pUrl) == 0)
						return 0;
				}
				memset(strOldUrl[nCurPos], 0, 50);
				strcpy(strOldUrl[nCurPos], pUrl);
				nCurPos++;
				nCurPos %= 5;
			}
			int nLen = strlen(pUrl);

			if (nLen >= nSize)
                return 0;
            if (nLen <= 5)
                return 0;
            return RecordUrl(pUrl,packet,ip_head->saddr ,ip_head->daddr);
        }
    }
    return 0;
}

static void * protocol_handler[2];
extern "C" int __module_init(struct so_data*so)
{
    //不是只有 80 端口才是 http !!!
    protocol_handler[0] = register_protocol_handler(url_packet_callback,0x5000, IPPROTO_TCP);
    protocol_handler[1] = register_protocol_handler(GetHttpPost,0x5000, IPPROTO_TCP);
    return 0;
}

extern "C" int so_can_unload()
{
    un_register_protocol_handler( protocol_handler[0] );
    un_register_protocol_handler( protocol_handler[1] );

	sleep(5);
	return 1;
}

char module_name[]="URL分析";
