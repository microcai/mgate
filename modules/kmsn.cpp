
//
// C++ Implementation: kmsn
//
// Description:
//
//
// Author:  <king@tao.kingstone>, (C) 2009
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include <net/ethernet.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <stdio.h>
#include <iostream>
#include <string.h>

#include "libmicrocai.h"
#include "kmysql.h"

#define MSN_PORT  0x4707  //1863


#define EVENT_FTP		"100"
#define EVENT_POP		"103"
#define EVENT_QQ		"1002"
#define EVENT_ICQ		"105"
#define EVENT_YAHOO		"106"
#define EVENT_HTTP		"0001"
#define EVENT_BF		"2004"
#define EVENT_LZ		"2001"
#define EVENT_WEBMAIL	        "0006"
#define EVENT_MN		"1001"

std::string Type_MSN("1001");

static char * MemStr(char *p1, const char *p2, int nCount)
{
    int nLen = strlen(p2);
    for (char *p = p1; p <= p1 + nCount - nLen; p++) {
        if ((*p == *p2) && (memcmp(p, p2, nLen) == 0))
            return p;
    }
    return NULL;
}

static int RecordMSNAccount(std::string msn,in_addr_t ip,in_addr_t dst_ip,u_char*packet)
{
    std::cout << msn << std::endl;
    struct NetAcount na(NetAcountType_MSN,packet);
    struct tcphdr* tcp = (tcphdr*)(packet + 14 + sizeof(iphdr));

    strcpy(na.strType,Type_MSN.c_str());
    na.data = msn;
    na.ip = ip;
    na.dstip = dst_ip;
    na.dport = ntohs(tcp->dest);
    RecordAccout(&na);
    return 1;
}
static int	FunctionInUse=0;
static int msn_packet_callback(struct so_data* sodata, u_char * packet)
{
	__sync_add_and_fetch(&FunctionInUse,1);
    /**************************************************
     *IP数据包通常就在以太网数据头的后面。以太网头的大小为14字节*
     **************************************************/
    struct iphdr * ip_head = (struct iphdr*) (packet + ETH_HLEN);
    /**************************************************
     *TCP头也就在IP头的后面。IP头的大小为20字节，不过最好从头里读取
     **************************************************/
    struct tcphdr * tcp_head = (struct tcphdr*) (packet + 14 + ip_head->ihl * 4);
    /**************************************************
     *TCP数据现对于tcp头的偏移由doff给出。这个也是tcp头的大小**
     **************************************************/
    char* tcpdata = (char*) tcp_head + tcp_head->doff * 4;
    int tcpdatelen = ntohs(ip_head->tot_len) - tcp_head->doff * 4 - ip_head->ihl * 4;
    /*太小的包肯定就不是*/
    if (tcpdatelen < 5)return 0;

    char strMSN[4960];
    int nSize=sizeof(strMSN);

    /*查找特定的MSN命令 USR */
    char *pData = MemStr((char *) tcpdata, "USR", tcpdatelen);
    //末尾置0避免出错
    tcpdata[tcpdatelen - 1] = 0;

    /*查找帐号*/
    if (pData) {
        while (1) {
            char *pTemp = strstr(pData + 1, " ");

            if (pTemp && pTemp > pData)
                pData = pTemp;
            else
                break;
        }
        while (pData && *pData == ' ')
            pData++;
        int nLen = tcpdatelen - (pData - tcpdata);
        if (nLen <= 0 || nLen >= nSize)
            return 0;
        memcpy(strMSN, pData, nLen - 2);

        if (strstr(strMSN, "@"))
        {
            //记录
            return RecordMSNAccount(strMSN,ip_head->saddr,ip_head->daddr,packet);
        }
    }
    return __sync_sub_and_fetch(&FunctionInUse,1);
}
static void * protocol_handler;
extern "C" int __module_init(struct so_data*so)
{
    protocol_handler = register_protocol_handler(msn_packet_callback,MSN_PORT, IPPROTO_TCP);
    return 0;
}
extern "C" int so_can_unload()
{
	return 1;
}

static void __attribute__((destructor)) so__unload(void)
{
	un_register_protocol_handler(protocol_handler);
	sleep(4);
}

char module_name[]="MSN号码分析";
