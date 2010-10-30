/*
 * unzip.h 支持zip解压
 *
 *      Copyright 2010 薇菜工作室
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *      MA 02110-1301, USA.
 */

#ifndef UNZIP_H_
#define UNZIP_H_

#include <zlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#undef __PACKED__
#define __PACKED__ __attribute__((packed))

#ifdef __LITTLE_ENDIAN
#define ztohs(x)	(x)
#define ztohl(x)	(x)
#else
#define ztohs(x)	__bswap_32(x)
#define ztohl(x)	__bswap_16(x)
#endif

typedef struct zipRecord{
	guint32 magic; // ztohl(0x04034b50)
	guint16 unzip_version; //解压所需要的版本
	guint16 global_flag;  // when 0x4 set , have DataDesc
	guint16 deflate_type;
	guint16	modifiedtime;
	guint16 modifieddate;
	guint32	crc;
	guint32	size_ziped;
	guint32 size_orig;
	guint16 filename_len;
	guint16 extra_len;
//========================
	gchar	filename[0];
	//fllow filename and extra info
}__PACKED__ zipRecord ;


typedef struct DataDesc{
	guint32	crc;
	guint32	size_ziped;
	guint32 size_orig;
}DataDesc;

const zipRecord* zipbuffer_search(gconstpointer data, gconstpointer data_end, const char * file);
void unzip_buffer(char * dst, gsize * dstlen ,const zipRecord * rc);

#ifdef __cplusplus
}
#endif

#endif /* UNZIP_H_ */
