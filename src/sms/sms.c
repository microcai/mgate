/*
 * sms.c
 *
 *  Created on: 2010-10-12
 *      Author: cai
 */
#include <string.h>
#include <glib.h>
#include "sms.h"



// 7bit编码
// pSrc: 源字符串指针
// pDst: 目标编码串指针
// nSrcLength: 源字符串长度
// 返回: 目标编码串长度
static int gsmEncode7bit(const char* pSrc, unsigned char* pDst, int nSrcLength)
{
	int nSrc; // 源字符串的计数值
	int nDst; // 目标编码串的计数值
	int nChar; // 当前正在处理的组内字符字节的序号，范围是0-7
	unsigned char nLeft; // 上一字节残余的数据

	// 计数值初始化
	nSrc = 0;
	nDst = 0;

	// 将源串每8个字节分为一组，压缩成7个字节
	// 循环该处理过程，直至源串被处理完
	// 如果分组不到8字节，也能正确处理
	while(nSrc<nSrcLength)
	{
		// 取源字符串的计数值的最低3位
		nChar = nSrc & 7;

		// 处理源串的每个字节
		if(nChar == 0)
		{
			// 组内第一个字节，只是保存起来，待处理下一个字节时使用
			nLeft = *pSrc;
		}
		else
		{
			// 组内其它字节，将其右边部分与残余数据相加，得到一个目标编码字节
			*pDst = (*pSrc << (8-nChar)) | nLeft;

			// 将该字节剩下的左边部分，作为残余数据保存起来
			nLeft = *pSrc >> nChar;

			// 修改目标串的指针和计数值
			pDst++;
			nDst++;
		}

		// 修改源串的指针和计数值
		pSrc++;
		nSrc++;
	}

	// 返回目标串长度
	return nDst;
}

// 8bit编码
// pSrc: 源字符串指针
// pDst: 目标编码串指针
// nSrcLength: 源字符串长度
// 返回: 目标编码串长度
static int gsmEncode8bit(const char* pSrc, unsigned char* pDst, int nSrcLength)
{	// 简单复制
	memcpy(pDst, pSrc, nSrcLength);
	return nSrcLength;
}


// UCS2编码
// pSrc: 源字符串指针
// pDst: 目标编码串指针
// nSrcLength: 源字符串长度
// 返回: 目标编码串长度
static int gsmEncodeUcs2(const char* pSrc, unsigned char* pDst, int nSrcLength)
{
	gunichar2 * wchar;	// UNICODE串
	glong read,nDstLength; // UNICODE宽字符数目

	// 字符串-->UNICODE串
	wchar = g_utf8_to_utf16(pSrc, nSrcLength, &read, &nDstLength,NULL);

	// 高低字节对调，输出
	for(int i=0; i<nDstLength; i++)
	{
		*pDst++ = wchar[i] >> 8;		// 先输出高位字节
		*pDst++ = wchar[i] & 0xff;		// 后输出低位字节
	}

	g_free(wchar);
	// 返回目标编码串长度
	return nDstLength * 2;
}

// 字节数据转换为可打印字符串
// 如：{0xC8, 0x32, 0x9B, 0xFD, 0x0E, 0x01} --> "C8329BFD0E01"
// pSrc: 源数据指针
// pDst: 目标字符串指针
// nSrcLength: 源数据长度
// 返回: 目标字符串长度
static int gsmBytes2String(const unsigned char* pSrc, char* pDst, int nSrcLength)
{
	const char tab[]="0123456789ABCDEF";	// 0x0-0xf的字符查找表
	for(int i=0; i<nSrcLength; i++)
	{
		*pDst++ = tab[*pSrc >> 4];		// 输出低4位
		*pDst++ = tab[*pSrc & 0x0f];	// 输出高4位
		pSrc++;
	}

	// 输出字符串加个结束符
	*pDst = '\0';

	// 返回目标字符串长度
	return nSrcLength * 2;
}

// 正常顺序的字符串转换为两两颠倒的字符串，若长度为奇数，补'F'凑成偶数
// pSrc: 源字符串指针
// pDst: 目标字符串指针
// nSrcLength: 源字符串长度
// 返回: 目标字符串长度
static int gsmInvertNumbers(const char* pSrc, char* pDst, int nSrcLength)
{
	int nDstLength;		// 目标字符串长度
	char ch;			// 用于保存一个字符

	// 复制串长度
	nDstLength = nSrcLength;

	// 两两颠倒
	for(int i=0; i<nSrcLength;i+=2)
	{
		ch = *pSrc++;		// 保存先出现的字符
		*pDst++ = *pSrc++;	// 复制后出现的字符
		*pDst++ = ch;		// 复制先出现的字符
	}

	// 源串长度是奇数吗？
	if(nSrcLength & 1)
	{
		*(pDst-2) = 'F';	// 补'F'
		nDstLength++;		// 目标串长度加1
	}

	// 输出字符串加个结束符
	*pDst = '\0';

	// 返回目标字符串长度
	return nDstLength;
}

int gsmEncodePdu(const SM_PARAM* pSrc, char* pDst)
{
	int nLength;			// 内部用的串长度
	int nDstLength;			// 目标PDU串长度
	unsigned char buf[256];	// 内部用的缓冲区

	// SMSC地址信息段
	nLength = strlen(pSrc->SCA);	// SMSC地址字符串的长度
	buf[0] = (char)((nLength & 1) == 0 ? nLength : nLength + 1) / 2 + 1;	// SMSC地址信息长度
	buf[1] = 0x91;		// 固定: 用国际格式号码
	nDstLength = gsmBytes2String(buf, pDst, 2);		// 转换2个字节到目标PDU串
	nDstLength += gsmInvertNumbers(pSrc->SCA, &pDst[nDstLength], nLength);	// 转换SMSC号码到目标PDU串

	// TPDU段基本参数、目标地址等
	nLength = strlen(pSrc->TPA);	// TP-DA地址字符串的长度
	buf[0] = 0x11;					// 是发送短信(TP-MTI=01)，TP-VP用相对格式(TP-VPF=10)
	buf[1] = 0;						// TP-MR=0
	buf[2] = (char)nLength;			// 目标地址数字个数(TP-DA地址字符串真实长度)
	buf[3] = 0x91;					// 固定: 用国际格式号码
	nDstLength += gsmBytes2String(buf, &pDst[nDstLength], 4);		// 转换4个字节到目标PDU串
	nDstLength += gsmInvertNumbers(pSrc->TPA, &pDst[nDstLength], nLength);	// 转换TP-DA到目标PDU串

	// TPDU段协议标识、编码方式、用户信息等
	nLength = strlen(pSrc->TP_UD);	// 用户信息字符串的长度
	buf[0] = pSrc->TP_PID;			// 协议标识(TP-PID)
	buf[1] = pSrc->TP_DCS;			// 用户信息编码方式(TP-DCS)
	buf[2] = 0;						// 有效期(TP-VP)为5分钟
	if(pSrc->TP_DCS == GSM_7BIT)
	{
		// 7-bit编码方式
		buf[3] = nLength;			// 编码前长度
		nLength = gsmEncode7bit(pSrc->TP_UD, &buf[4], nLength+1) + 4;	// 转换TP-DA到目标PDU串
	}
	else if(pSrc->TP_DCS == GSM_UCS2)
	{
		// UCS2编码方式
		buf[3] = gsmEncodeUcs2(pSrc->TP_UD, &buf[4], nLength);	// 转换TP-DA到目标PDU串
		nLength = buf[3] + 4;		// nLength等于该段数据长度
	}
	else
	{
		// 8-bit编码方式
		buf[3] = gsmEncode8bit(pSrc->TP_UD, &buf[4], nLength);	// 转换TP-DA到目标PDU串
		nLength = buf[3] + 4;		// nLength等于该段数据长度
	}
	nDstLength += gsmBytes2String(buf, &pDst[nDstLength], nLength);		// 转换该段数据到目标PDU串

	// 返回目标字符串长度
	return nDstLength;
}

