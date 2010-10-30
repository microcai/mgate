/*
 * ksql.h
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

#ifndef KSQL_H_
#define KSQL_H_

#include <glib-object.h>
#include "gsqlresult.h"

void	ksql_init(gboolean createdb) ;

GSQLResult * ksql_query(const gchar * stm);
GSQLResult * ksql_query_free_str(gchar * stm);
GSQLResult * ksql_vquery(const gchar * stmformat , ...);
void ksql_query_async(const gchar * stm);
void ksql_query_async_free_str(const gchar * stm);
void ksql_vquery_async(const gchar * stmformat , ...);

#endif /* KSQL_H_ */
