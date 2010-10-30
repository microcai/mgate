/*
 * getmime.c -- 获得文件的 MIME 类型
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
#include <stdio.h>
#include <string.h>

const char * getext(const char * name)
{
	const char * p = name + strlen(name);
	while (*p != '.' && p > name)
		--p;
	return p + 1;
}

const char * getmime_by_filename(const char * filename)
{
//	char line[200];
	static char line[200];
	const char * type = "application/octet-stream";
	FILE * f = fopen("/etc/mime.types","r");

	if(!f)
		return type;

	const char * ext = getext(filename);

	while(!feof(f))
	{
		char * saveptr;
		char * tok;
		fgets(line,sizeof(line),f);

		tok = strtok_r(line," \t\n\r",&saveptr);

		if(tok == 0  || tok[0]=='#')
			continue;

		type = tok;

		tok = strtok_r(NULL," \t\n\r",&saveptr);

		while(tok)
		{
			if(strcasecmp(tok,ext)==0)
			{
				fclose(f);
				return type;
			}
			tok = strtok_r(NULL," \t\n\r",&saveptr);
		}
	}
	fclose(f);
	return type;
}
