/*
 * utils.h
 *
 *  Created on: 2010-4-8
 *      Author: cai
 */

#ifndef UTILS_H_
#define UTILS_H_

#include <glib.h>

G_BEGIN_DECLS

guint64	mac2uint64( guchar mac[6]);

int gbk_utf8(char *outbuf, size_t outlen, const char *inbuf, size_t inlen);
int utf8_gbk(char *outbuf, size_t outlen, const char *inbuf, size_t inlen);
double GetDBTime_str(char *pTime);
double GetDBTime_tm(struct tm * ptm);

G_END_DECLS


#endif /* UTILS_H_ */
