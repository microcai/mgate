/*
 * CString.cpp
 *
 *  Created on: 2009-7-6
 *      Author: root
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "CString.h"

CString::CString()
{
	m_len = 16;
	m_strlen = 0;
	m_data = (typeof(m_data)) malloc(m_len);
}

CString::CString(const char *str)
{

	m_len = 64;
	m_data = (typeof(m_data)) malloc(m_len);
	setstr(str);
}

CString::~CString()
{
	free(m_data);
}

CString & CString::Format(const char * fmt, ...)
{
	va_list ap;
	va_start(ap,fmt);
	(void)vFormat(fmt,ap);
	va_end(ap);
	return *this;
}

CString & CString::vFormat(const char * fmt, va_list ap)
{
	if(m_len < 4096)
	{
		free(m_data);
		m_data = (typeof(m_data)) malloc(m_len = 4096);
	}
	m_strlen = vsnprintf(m_data, m_len, fmt, ap);
	return *this;
}



CString::operator const char *()
{
	return (const char*) m_data;
}

CString::operator const char *() const
{
	return (const char*) m_data;
}

const char * CString::c_str()
{
	return m_data;
}

const char * CString::c_str() const
{
	return m_data;
}
void CString::alloc(size_t s)
{
	free(m_data);
	m_len = s;
	m_data = (typeof(m_data)) malloc(m_len);
}

void CString::setstr(const char * that)
{
	size_t s;

	s = strlen(that);

	if (s > m_len)
	{
		m_len += ((s >> 4) << 4) + 16;
		free(m_data);
		m_data = (typeof(m_data)) malloc(m_len);
	}
	memcpy(m_data, that, s + 1);
	m_strlen = s;
}

CString &CString::operator =(CString & that)
{
	if (m_len <= that.m_strlen)
	{
		free(m_data);
		m_data = (typeof(m_data)) malloc(that.m_len);
		m_len = that.m_len;
	}
	m_strlen = that.m_strlen;
	memcpy(m_data, that.m_data, m_strlen + 1);
	return *this;
}

CString & CString::operator =(const char*that)
{
	size_t s;

	s = strlen(that);

	if( s  > m_len )
	{
		m_len += ((s >> 4) << 4) + 16;
		free(m_data);
		m_data = (typeof(m_data))malloc(m_len);
	}
	memcpy(m_data , that, s +1);
	m_strlen = s ;
	return * this;
}

CString & CString::operator +=(const char * app)
{
	m_data[m_len-1] = 0;

	size_t s = strlen(app);

	if( (m_strlen + s) >= m_len )
	{
		m_len = (((s - m_len + m_strlen) >> 4) << 4) + 16;
		free(m_data);
		m_data = (typeof(m_data)) malloc(m_len);
	}
	memcpy(m_data + m_strlen, app, s + 1);
	m_strlen += s;
	return *this;
}

CString & CString::operator +=(std::string & app)
{
	m_data[m_len - 1] = 0;

	size_t s = app.length();

	if ((m_strlen + s) >= m_len)
	{
		m_len = (((s - m_len + m_strlen) >> 4) << 4) + 16;
		free(m_data);
		m_data = (typeof(m_data)) malloc(m_len);
	}
	memcpy(m_data + m_strlen, app.c_str(), s + 1);
	m_strlen += s;
	return *this;
}

CString & CString::operator +=(CString& app)
{
	m_data[m_len - 1] = 0;

	size_t s = app.m_strlen;

	if ((m_strlen + s) >= m_len)
	{
		m_len = (((s - m_len + m_strlen) >> 4) << 4) + 16;
		free(m_data);
		m_data = (typeof(m_data)) malloc(m_len);
	}
	memcpy(m_data + m_strlen, app.c_str(), s + 1);
	m_strlen += s;
	return *this;
}
CString CString::operator +(const char * app)
{
	CString tmp(*this);
	return tmp+=app;
}

CString operator +(CString & a , CString & b )
{
	CString tmp(a);
	return tmp+=b;
}
int gbk_utf8(char *outbuf, size_t outlen, const char *inbuf, size_t inlen);
CString& CString::from_gbk(const char* gbk,size_t len)
{
	if(len<0)
		len = strlen(gbk);
	alloca( len*2 );
	gbk_utf8(this->m_data, m_len, gbk, len);
	m_strlen = strlen(m_data);
	return *this;
}


