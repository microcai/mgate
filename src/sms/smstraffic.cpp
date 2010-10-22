// SmsTraffic.cpp: implementation of the CSmsTraffic class.
//
//////////////////////////////////////////////////////////////////////

#include "smstraffic.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CSmsTraffic::CSmsTraffic(CComm* pcomm)
{
	m_nSendIn = 0;
	m_nSendOut = 0;
	m_nRecvIn = 0;
	m_nRecvOut = 0;
	
	m_pComm=pcomm;
	m_gsm.m_pComm=m_pComm;

	m_hKillThreadEvent = ::CreateEvent(NULL, TRUE, FALSE, NULL);
	m_hThreadKilledEvent = ::CreateEvent(NULL, TRUE, FALSE, NULL);

	::InitializeCriticalSection(&m_csSend);
	::InitializeCriticalSection(&m_csRecv);

	// 启动子线程
	::AfxBeginThread(SmThread, this, THREAD_PRIORITY_NORMAL);
}

CSmsTraffic::~CSmsTraffic()
{
	::SetEvent(m_hKillThreadEvent);			
	::WaitForSingleObject(m_hThreadKilledEvent, INFINITE);	

	::DeleteCriticalSection(&m_csSend);
	::DeleteCriticalSection(&m_csRecv);

	::CloseHandle(m_hKillThreadEvent);
	::CloseHandle(m_hThreadKilledEvent);
}

// 将一条短消息放入发送队列
void CSmsTraffic::PutSendMessage(SM_PARAM* pSmParam)
{
	SM_PARAM* pSP=new SM_PARAM;
	if(pSP==NULL)//分配内存失败则不超作
	{
		return;
	}
	::memcpy(pSP, pSmParam, sizeof(SM_PARAM));
	::EnterCriticalSection(&m_csSend);
	//添加到指针链表。
		m_ptrlistSndMsg.AddTail(pSP);
	::LeaveCriticalSection(&m_csSend);
}

// 从发送队列中取一条短消息
BOOL CSmsTraffic::GetSendMessage(SM_PARAM* pSmParam)
{
	BOOL fSuccess = FALSE;
	SM_PARAM* pP=NULL;
	::EnterCriticalSection(&m_csSend);

	if(!m_ptrlistSndMsg.IsEmpty())
	{//如果有待发短信，取出一条，并且释放内存
		pP=(SM_PARAM*)m_ptrlistSndMsg.RemoveHead();
		fSuccess = TRUE;
	}
	::LeaveCriticalSection(&m_csSend);
	if(pP==NULL)
		return FALSE;
	::memcpy(pSmParam, pP, sizeof(SM_PARAM));
	delete pP;

	return fSuccess;
}

// 将一条短消息放入接收队列
void CSmsTraffic::PutRecvMessage(SM_PARAM* pSmParam)
{
	::EnterCriticalSection(&m_csRecv);

	::memcpy(&m_SmRecv[m_nRecvIn], pSmParam, sizeof(SM_PARAM));

	m_nRecvIn++;
	if(m_nRecvIn >= MAX_SM_RECV)  m_nRecvIn = 0;

	::LeaveCriticalSection(&m_csRecv);
}

// 从接收队列中取一条短消息
BOOL CSmsTraffic::GetRecvMessage(SM_PARAM* pSmParam)
{
	BOOL fSuccess = FALSE;

	::EnterCriticalSection(&m_csRecv);

	if(m_nRecvOut != m_nRecvIn)
	{

		::memcpy(pSmParam, &m_SmRecv[m_nRecvOut], sizeof(SM_PARAM));

		m_nRecvOut++;
		if(m_nRecvOut >= MAX_SM_RECV)  m_nRecvOut = 0;

		fSuccess = TRUE;
	}

	::LeaveCriticalSection(&m_csRecv);

	return fSuccess;
}


UINT CSmsTraffic::SmThread(LPVOID lParam)
{
	CSmsTraffic* p=(CSmsTraffic *)lParam;	// this
	int nMsg;					// 收到短消息条数
	int nDelete;				// 目前正在删除的短消息编号
	int i;
	int m_nSndTrys=0;
	int m_nDltTrys=0;
	SM_PARAM SmParam[128];		// 短消息缓冲区
	enum {
		stHaveRest,				// 休息，延时
		stGetSendMessage,		// 取一条待发送的短消息
		stDoSendMessage,		// 发送短消息
		stSendWaitIdle,			// 发送不成功，等待GSM就绪
		stDoRecvMessage,		// 接收短消息
		stPutRecvMessage,		// 储存接收的短消息
		stDeleteMessage,		// 删除短消息
		stDeleteWaitIdle,		// 删除不成功，等待GSM就绪
		stExit					// 退出
	} nState;					// 处理过程的状态

	nState = stHaveRest;

	// 发送和接收
	while(nState != stExit)
	{
		switch(nState)
		{
			case stHaveRest:
				::Sleep(1000);
				nState = stGetSendMessage;
				break;
			case stGetSendMessage:
				if(p->GetSendMessage(&SmParam[0]))  nState = stDoSendMessage;
				else nState = stDoRecvMessage;
				break;
			case stDoSendMessage:
				if(p->m_gsm.gsmSendMessage(&SmParam[0]))  
				{//如果发成功了，等4500毫秒
				//	Sleep(4500);
					nState = stDoRecvMessage;
					m_nSndTrys=0;
				}
				else nState = stSendWaitIdle;
				break;
			case stSendWaitIdle:
				::Sleep(1000);
				if(m_nSndTrys++>3)//失败太多次了，放弃该条短信。
				{	
					
					p->m_pComm->ClearCommEr();
					p->m_pComm->WriteComm("AT+CMGF=0\r\n",10);
					nState=stHaveRest;
				}
				else
					nState = stDoSendMessage;
				break;
			case stDoRecvMessage:
				nMsg = p->m_gsm.gsmReadMessage(SmParam);
				if(nMsg > 0)  nState = stPutRecvMessage;
				else nState = stHaveRest;
				break;
			case stPutRecvMessage:
				for(i = 0; i < nMsg; i++)  p->PutRecvMessage(&SmParam[i]);
				nState = stDeleteMessage;
				nDelete = 0;
				break;
			case stDeleteMessage:
				if(nDelete < nMsg)
				{
					if(p->m_gsm.gsmDeleteMessage(SmParam[nDelete].index)) 
					{//继续循环删除
						nState=stDeleteMessage;
						nDelete++;
						m_nDltTrys=0;
					}
					else nState = stDeleteWaitIdle;
				}
				else  nState = stHaveRest;
				break;
			case stDeleteWaitIdle:
				::Sleep(1000);
				if(m_nDltTrys++>3)
				{
					p->m_pComm->ClearCommEr();
					p->m_pComm->WriteComm("AT+CMGF=0\r\n",10);
					nState=stHaveRest;
				}
				else
					nState = stDeleteMessage;
				break;
		}

		// 检测是否有关闭本线程的信号
		DWORD dwEvent = ::WaitForSingleObject(p->m_hKillThreadEvent, 20);
		if(dwEvent == WAIT_OBJECT_0)  nState = stExit;
	}

	// 置该线程结束标志
	::SetEvent(p->m_hThreadKilledEvent);

	return 9999;
}
