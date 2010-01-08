/*
 * CString.h
 *
 *  Created on: 2009-7-6
 *      Author: root
 */

#ifndef CSTRING_H_
#define CSTRING_H_

#include <stdarg.h>
#include <stddef.h>
#include <string>

class CString
{
public:
	CString();
	CString(const char * str);
	~CString();
public:
	operator const char *();
	operator const char *() const;
	const char * c_str();
	const char * c_str() const;
public:
	CString& Format(const char * fmt, ...);
	CString& Format(const char * fmt, va_list);
	CString& operator =(CString&);
	CString& operator =(const char *);
	CString& operator +=(CString &);
	CString& operator +=(const char *);
	CString& operator +=(std::string &);
	CString operator +(const char *);

protected:
	void setstr(const char *);
protected:
	char * m_data;
	size_t m_len; // block len
	size_t m_strlen;
};

CString operator +(CString &, CString &);

#endif /* CSTRING_H_ */
