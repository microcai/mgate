/*
 * unzip.c
 *
 *  Created on: 2010-10-22
 *      Author: cai
 */


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <string.h>
#include <glib.h>
#include <zlib.h>
#include "unzip.h"

//寻找压缩包里的文件
const zipRecord* zipbuffer_search(gconstpointer data, gconstpointer data_end, const char * file)
{
	const char * ptr = data;
	const zipRecord * ziprec = (typeof(ziprec))ptr;

	int filename_len = strlen(file);


	while (ziprec->magic == ztohl(0x04034b50) &&
			(((gsize)ziprec) < ((gsize)data_end)))
	{
		if(filename_len == ziprec->filename_len && strncasecmp(ziprec->filename,file,filename_len)==0)
			return ziprec;

		ptr += sizeof(zipRecord) + ziprec->filename_len + ziprec->extra_len + ziprec->size_ziped;
		ptr += (ziprec->global_flag & 0x4)? sizeof(DataDesc):0;

		ziprec = (typeof(ziprec))ptr;
	}

	return NULL;
}

void unzip_buffer(char * dst, gsize * dstlen , const zipRecord * rc)
{
	z_stream strm[1];

	memset(strm,0,sizeof(strm));

	inflateInit2(strm,-MAX_WBITS);

	strm->avail_in = rc->size_ziped;
	strm->avail_out = *dstlen;

	strm->next_in = (Bytef *)rc + sizeof(zipRecord) + rc->filename_len + rc->extra_len;
	strm->next_out = (Bytef *)dst;

	inflate(strm,Z_FINISH);

	*dstlen -= strm->avail_out;

	inflateEnd(strm);
}

