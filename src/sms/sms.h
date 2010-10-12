// Comm.h: interface for the CComm class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(SMS_H__)
#define SMS_H__

// 用户信息编码方式
#define GSM_7BIT		0
#define GSM_8BIT		4
#define GSM_UCS2		8

// 短消息参数结构，编码/解码共用
// 其中，字符串以0结尾
typedef struct _SM_PARAM{
	char SCA[16];			// 短消息服务中心号码(SMSC地址)
	char TPA[16];			// 目标号码或回复号码(TP-DA或TP-RA)
	char TP_PID;			// 用户信息协议标识(TP-PID)
	char TP_DCS;			// 用户信息编码方式(TP-DCS)
	char TP_SCTS[16];		// 服务时间戳字符串(TP_SCTS), 接收时用到
	char TP_UD[161];		// 原始用户信息(编码前或解码后的TP-UD)
	char index;				// 短消息序号，在读取时用到
} SM_PARAM;

// PDU编码，用于编制、发送短消息
// 返回: 目标PDU串长度
int gsmEncodePdu(const SM_PARAM* pSrc, char* pDst);


#endif // !defined(SMS_H__)
