/*
 * getmime.c
 *
 *  Created on: 2010-10-22
 *      Author: cai
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
