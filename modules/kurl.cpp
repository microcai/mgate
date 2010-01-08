
#include <iostream>
#include <map>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <net/ethernet.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <vector>
#include <stdio.h>

#include "libdreamtop.h"
#include "kmysql.h"

#include <ctime>
std::map<std::string, time_t> url_time_map;

struct MAIL_LOGIN_KEY
{
	char user[16];
	char password[16];
	char select[16];
};

static char CharToInt(char ch){
	if(ch>='0' && ch<='9')return (char)(ch-'0');
	if(ch>='a' && ch<='f')return (char)(ch-'a'+10);
	if(ch>='A' && ch<='F')return (char)(ch-'A'+10);
	return -1;
}

static char StrToBin(char *str){
	char tempWord[2];
	char chn;

	tempWord[0] = CharToInt(str[0]);                         //make the B to 11 -- 00001011
	tempWord[1] = CharToInt(str[1]);                         //make the 0 to 0 -- 00000000

	chn = (tempWord[0] << 4) | tempWord[1];                //to change the BO to 10110000

	return chn;
}

static void UrlGB2312Decode(const char *pInput,int nInputLen,char *pOutput,int nOutputLen)
{
	char tmp[2];
	int i=0;

	while(i<nInputLen)
	{
		if(pInput[i]=='%')
		{
			tmp[0]=pInput[i+1];
			tmp[1]=pInput[i+2];
			pOutput[strlen(pOutput)]= StrToBin(tmp);
			if(strlen(pOutput)>63)
			{
				fprintf(stderr,"UrlGB2312Decode,pOutput over\n");
			}

			i=i+3;
		}
		else if(pInput[i]=='+'){
			pOutput[strlen(pOutput)]=' ';
			i++;
			if(strlen(pOutput)>63)
			{
				fprintf(stderr,"UrlGB2312Decode,pOutput over\n");
			}
		}
		else{
			pOutput[strlen(pOutput)]=pInput[i];
			i++;
			if(strlen(pOutput)>63)
			{
				fprintf(stderr,"UrlGB2312Decode,pOutput over\n");
			}
		}
	}
}
// Parameter sep is char set
size_t GetSepTextByChar(const std::string &strText, const char *sep, std::vector<std::string> &vec)
{
	if (sep == NULL)
		return 0;
	size_t startpos = 0, endpos =0;

	while(startpos != (size_t)-1)
	{
		startpos = strText.find_first_not_of(sep, startpos);
		if (startpos ==  (size_t)-1)
			break;

		endpos = strText.find_first_of(sep, startpos);

		std::string::size_type end = (endpos ==  (size_t) -1)?strText.size():endpos;
		std::string temp = strText.substr(startpos, end - startpos);

		if (!temp.empty())
			vec.push_back(temp);
		startpos = endpos;
	}
	return vec.size();
}

bool ParseTcpPkt(std::string strIndex, const char* tcpdata, size_t tcpdata_len, std::string& url, std::string& usr, std::string& pwd)
{
	static std::map<std::string, std::string> ip_html_map;
	if (ip_html_map.size() >10000)
		ip_html_map.clear();

	std::string strdata(tcpdata, tcpdata_len);

	//data begin with "POST"
	if (strdata.find("POST") != 0 && (ip_html_map.find(strIndex) == ip_html_map.end() ||ip_html_map[strIndex] == ""))
		return false;

	if (strdata.find("POST") == 0 || ip_html_map[strIndex].size() >4*1024)
		ip_html_map[strIndex] = "";

	ip_html_map[strIndex] += strdata;

	size_t pos = std::string::npos;
	if ((pos = ip_html_map[strIndex].find("\r\n\r\n", 0)) == std::string::npos)
		return false;

	std::string html_header = ip_html_map[strIndex].substr(0, pos+4);
	std::string html_body = ip_html_map[strIndex].substr(pos+4, strdata.size() -pos-4);

	//get the html field "Content Length"
	size_t p1 = std::string::npos, p2 = std::string::npos;
	if ((p1 = html_header.find("Content-Length: ")) == std::string::npos || 
		(p2 = html_header.find("\r\n", p1)) == std::string::npos)
	{
		ip_html_map[strIndex] = "";
		return false;
	}
	p1 += strlen("Content-Length: ");
	std::string content_len = html_header.substr(p1, p2-p1);
	int len = atoi(content_len.c_str());

	if (len> html_body.size())
		return false;

	ip_html_map[strIndex] = "";

	//url
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
	keyword_map.insert(std::make_pair("name", "pwd"));
	keyword_map.insert(std::make_pair("vwriter", "vpassword"));
	keyword_map.insert(std::make_pair("email", "password"));

	std::multimap<std::string, std::string>::iterator it_map = keyword_map.begin();
	for (; it_map != keyword_map.end(); ++it_map)
	{
		if(usr_pwd_map.find(it_map->first) != usr_pwd_map.end() &&
			usr_pwd_map.find(it_map->second) != usr_pwd_map.end())
		{
			usr = usr_pwd_map[it_map->first];
			pwd = usr_pwd_map[it_map->second];

			char szUser[64] = {0};
			char szPassword[64] = {0};
			UrlGB2312Decode(usr.c_str(), usr.size(), szUser, strlen(szUser));
			UrlGB2312Decode(pwd.c_str(), pwd.size(), szPassword, strlen(szPassword));

			usr = szUser;
			pwd = szPassword;
			return true;
		}
	}

	return false;	
}


static int RecordPOST(const char *user, const char*pswd, const char*host, u_char*packet,in_addr_t sip,in_addr_t dip)
{
	struct NetAcount na(NetAcountType_POST,packet);// = (struct NetAcount*)malloc(8190);

	struct tcphdr* tcp = (tcphdr*)(packet + 14 + sizeof(iphdr));

	na.data = host;
	na.ip = sip;
	na.dstip = dip;

	strcpy(na.strType,"0006");

	char key2[80];
	snprintf(key2,80,"%s:%s",user,pswd);
	na.passwd = key2 ;
	na.host = host;
	na.dport = ntohs(tcp->dest);

	RecordAccout(&na);
	return 1;
}

static int RecordUrl(char *url,u_char*packet,in_addr_t sip,in_addr_t dip)
{
	std::string strUrl(url);

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
	return 0;
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
		if (memcmp("POST ", tcpdata, 5) == 0)
		{
			char *pPos = NULL;
			pPos = strstr((char *) tcpdata, "Host:");
			if (pPos)
			{

	//static pthread_mutex_t lock=PTHREAD_MUTEX_INITIALIZER;
	//pthread_mutex_lock(&lock);

	try
	{
		char strIndex[128] = {0};
		snprintf(strIndex, sizeof(strIndex), "%d%d%d%d",
			ip_head->daddr, 
			ip_head->saddr,
			tcp_head->dest,
			tcp_head->source);

		std::string url, usr, pwd;

		if(ParseTcpPkt(strIndex, (const char*)tcpdata, tcpdatelen, url, usr, pwd))
		{
			int nret =  RecordPOST(usr.c_str(), pwd.c_str(), url.c_str(), packet,ip_head->saddr, ip_head->daddr);
			//pthread_mutex_unlock(&lock);
			return nret;
		}

	}
	catch (...)
	{
		//pthread_mutex_unlock(&lock);
		return 0;
	return 0;
	}
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

    const int nSize = 300;
    char pUrl[300];//= new char[nSize];

    memset(pUrl, 0, nSize);
    static char strOldUrl[20][300];
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
				for (int n = 0; n < 20; n++)
				{
					if (strcmp(strOldUrl[n], pUrl) == 0)
						return 0;
				}

				int nLen = strlen(pUrl);

				if (nLen >= nSize)
					return 0;
				if (nLen <= 5)
				int nret = RecordUrl(pUrl,packet,ip_head->saddr ,ip_head->daddr);

				return nret;
			}
			catch (...)
			{
					return 0;
            return RecordUrl(pUrl,packet,ip_head->saddr ,ip_head->daddr);
				strcpy(strOldUrl[nCurPos], pUrl);
				nCurPos++;
				nCurPos %= 20;
			}


			try
			{
				int nret = RecordUrl(pUrl,packet,ip_head->saddr ,ip_head->daddr);

				return nret;
			}
			catch (...)
			{
				return 0;
			}
            
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
