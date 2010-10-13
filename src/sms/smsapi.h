// SmsAPI.h: interface for the CSmsAPI class.
//
//////////////////////////////////////////////////////////////////////
#ifndef ___SMS_API_H__
#define ___SMS_API_H__

#include <glib.h>

G_BEGIN_DECLS

gboolean sms_init();
gboolean sms_sendmessage(const gchar * phone,const char * message);

G_END_DECLS
#endif
