// SmsAPI.cpp: implementation of the CSmsAPI class.
//
//////////////////////////////////////////////////////////////////////


#include "smsapi.h"
#include "sms.h"
#include "smstraffic.h"

int m_nCommPort=1;
int m_nBaudRate=9600;
CString m_strMSCA="+8613800371500";
CComm m_comm;
CSmsTraffic* m_psmstraffic=NULL;

 int OpenComm()
{
try{	
	if(m_psmstraffic!=NULL)
		delete m_psmstraffic;
	CString com;
	com.Format("COM%d",m_nCommPort);
	if(!m_comm.OpenComm(com.GetBuffer(com.GetLength()),m_nBaudRate,0,8,0))
	{
		m_comm.hComm=NULL;
		return -1;//失败打开串口
	}
	char buf[128];
	m_comm.ClearCommEr();
	m_comm.WriteComm("AT+CGSN\r",8);//短信格式为：PDU。
	::Sleep(250);
	int n=m_comm.ReadComm(buf,128);
	if(n<10)
	{
		m_comm.CloseComm();
		m_comm.hComm=NULL;
		return -2;
	}
	
	m_comm.WriteComm("AT+CMGF=0\r",10);//短信格式为：PDU。
	::Sleep(50);
	m_comm.ReadComm(buf,128);
	CString strsmca="AT+CSCA="+m_strMSCA+"\r";
	m_comm.WriteComm(strsmca.GetBuffer(strsmca.GetLength()),strsmca.GetLength());
	::Sleep(80);
	m_comm.ReadComm(strsmca.GetBuffer(12),128);
	Sleep(200);
	m_psmstraffic=new CSmsTraffic(&m_comm);
	Sleep(1000);
//	StartTimer();

	return 0;//成功打开COMMPORT；	return 0;
	 }
	 	catch(...)
	{
		return -3;
	}
}
int CloseComm()
{
	try{	if(m_psmstraffic!=NULL)
		delete m_psmstraffic;
	if(m_comm.hComm!=NULL)
		m_comm.CloseComm();
	m_comm.hComm=NULL;
	}
		catch(...)
	{
		return 0;
	}
	return 0;
}
//收发短信
int SendMsg(LPCTSTR phonenumber,LPCTSTR msg)
{
	try{	
		if(m_psmstraffic==NULL)
			return -1;
		CString strMsg= msg;


		CString strSmsc="+8613800371500";
		if(!m_strMSCA.IsEmpty())
			strSmsc=m_strMSCA;
		CString strNumber=phonenumber;
		CString strContent=strMsg;
		SM_PARAM SmParam;

		::memset(&SmParam, 0, sizeof(SM_PARAM));

		// 去掉号码前的"+"
		if(strSmsc[0] == '+')  strSmsc = strSmsc.Mid(1);
		if(strNumber[0] == '+')  strNumber = strNumber.Mid(1);

		// 在号码前加"86"
		if(strSmsc.Left(2) != "86")  strSmsc = "86" + strSmsc;
		if(strNumber.Left(2) != "86")  strNumber = "86" + strNumber;

		// 填充短消息结构
		::strcpy(SmParam.SCA, strSmsc);
		::strcpy(SmParam.TPA, strNumber);
		::strcpy(SmParam.TP_UD, strContent);
		SmParam.TP_PID = 0;
		SmParam.TP_DCS = GSM_UCS2;

		// 发送短消息
		m_psmstraffic->PutSendMessage(&SmParam);	

//	this->m_smstraffic.PutSendMessage(&psmparam);
	return 0;
	}
	catch(...)
	{
		OutputDebugString("send sms error!");
		return -2;
	}
}

int ReadMsg(VARIANT * pvariantSMCA, VARIANT * pvariantOA, VARIANT * pvariantMSG, VARIANT * pvariantTIME)
{
	 try{	SM_PARAM m_msparam;
	if(m_psmstraffic!=NULL)
		if(m_psmstraffic->GetRecvMessage(&m_msparam))
		{
			if(m_msparam.index<0)
				m_msparam.index=0;
		}
		else
			m_msparam.index=-1;

	if(m_msparam.index==-1)
		return -1;
	CString strsmca,strmsg,stroa,strtime;
	strsmca=m_msparam.SCA;
	strmsg=m_msparam.TP_UD;
	stroa=m_msparam.TPA;
	strtime=m_msparam.TP_SCTS;

	pvariantSMCA->bstrVal=strsmca.AllocSysString();//(BSTR)wchar;
	pvariantOA->bstrVal=stroa.AllocSysString();//(BSTR)wchar1;
	pvariantMSG->bstrVal=strmsg.AllocSysString();//(BSTR)wchar2;
	pvariantTIME->bstrVal=strtime.AllocSysString();//(BSTR)wchar3;
	m_msparam.index=-1;
	return 0;
	 }
	 	catch(...)
	{
		return -2;
	}
}
//设置属性，必需在打开串口前调，设置好
int SetCommPort(int nPortNumber,int nBaudRate)//串口号和波特率，比如：1，9600
{
	m_nCommPort=nPortNumber;
	m_nBaudRate=nBaudRate;
	return 0;

}
int SetMSCA(CString strMSCA)//短信中心号码。
{
	m_strMSCA=strMSCA;
	return 0;
}
