// SmsAPI.h: interface for the CSmsAPI class.
//
//////////////////////////////////////////////////////////////////////
#ifndef ___SMS_API_H__
#define ___SMS_API_H__

#include <glib.h>

gboolean sms_init();
gboolean sms_sendmessage(const gchar * phone,const char * message);

#endif
