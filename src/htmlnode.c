/*
 * htmlnode.c - tools to generate html file
 *
 *  Created on: 2010-4-25
 *      Author: cai
 */

#include <string.h>
#include <glib.h>
#include "htmlnode.h"

HtmlNode * htmlnode_new(HtmlNode * parent, const char * tag , ...)
{
	va_list v;

	HtmlNode * node = g_new0(HtmlNode,1);

	node->parent = parent;

	if (parent)
		htmlnode_append_child(parent, node);

	if (tag)
	{
		node->tag = g_strdup(tag);
		va_start(v,tag);

		while ((tag = va_arg(v,char*)))
		{
			htmlnode_append_attr(node, tag);
		}

		va_end(v);
	}
	return node;
}

HtmlNode * htmlnode_newv(HtmlNode * parent, const char * tag, va_list v)
{
	HtmlNode * node = g_new0(HtmlNode,1);

	node->parent = parent;

	if(parent)
		htmlnode_append_child(parent,node);

	if(tag)
	{
		node->tag = g_strdup(tag);
		while ((tag = va_arg(v,char*)))
		{
			htmlnode_append_attr(node,tag);
		}
	}

	return node;
}

HtmlNode * htmlnode_new_text(HtmlNode * parent, const char * text)
{
	HtmlNode * node = htmlnode_new(parent,NULL);

	node->plane = g_strdup(text);

	return node;
}


HtmlNode * htmlnode_new_head(HtmlNode * parent, const char * attrlist, ...)
{

}

HtmlNode * htmlnode_new_body(HtmlNode * parent, const char * attrlist, ...)
{

}

HtmlNode * htmlnode_new_table(HtmlNode * parent,const char * attrlist, ...)
{
	va_list v;
	va_start(v,attrlist);
	return htmlnode_newv(parent,"table",v);
}

HtmlNode * htmlnode_new_tr(HtmlNode * parent, const char * attrlist, ...)
{

}

HtmlNode * htmlnode_new_td(HtmlNode * parent, const char * attrlist, ...)
{

}

HtmlNode * htmlnode_new_frame(HtmlNode * parent,const char * method , const char * action ,const char * attrlist, ...)
{
	va_list v;

	method = g_strdup_printf("method=\"%s\"",method);

	action = g_strdup_printf("action=\"%s\"",action);

	HtmlNode * node = htmlnode_new(parent,"frame",method,action);

	g_free((gpointer)method);
	g_free((gpointer)action);

	va_start(v,attrlist);

	while (attrlist)
	{
		htmlnode_append_attr(node,(const char *)attrlist);
		attrlist = va_arg(v,char*);
	}
	va_end(v);

}


HtmlNode * htmlnode_append_child(HtmlNode * node,HtmlNode * child)
{
	node->children = g_list_append(node->children,child);
	return node;
}

HtmlNode * htmlnode_append_attr(HtmlNode * node, const char * attr)
{
	node->attr = g_list_append(node->attr,g_strdup(attr));
	return node;
}

HtmlNode * htmlnode_append_attr_take(HtmlNode * node, char * attr)
{
	node->attr = g_list_append(node->attr,attr);
	return node;
}

gboolean htmlnode_to_plane_text_internal(HtmlNode * rootnode, htmlnode_appender appender, gpointer user_data , int depth,gboolean freenode)
{
	if (rootnode->tag)
	{
		gchar * kg;
		if (depth)
		{
			appender("\n", user_data);
			kg = g_malloc0(depth + 1);
			memset(kg, ' ', depth);
			appender(kg, user_data);
		}

		appender("<", user_data);

		appender(rootnode->tag, user_data);

		void appendlist(char * attrstr, gpointer user_data)
		{
			appender(" ", user_data);
			appender(attrstr, user_data);
		}

		if (rootnode->attr)
		{
			g_list_foreach(rootnode->attr, (GFunc) appendlist, user_data);
		}

		appender(">", user_data);

		void node_for(HtmlNode * node, gpointer user_data)
		{
			htmlnode_to_plane_text_internal(node, appender, user_data, depth + 1 , freenode);
		}

		if (rootnode->children)
		{
			g_list_foreach(rootnode->children, (GFunc) node_for, user_data);
		}

		if (depth)
		{
			appender(kg, user_data);

			g_free(kg);
		}

		appender("</", user_data);
		appender(rootnode->tag, user_data);
		appender(">\n", user_data);
	}
	else
	{
		appender(rootnode->plane, user_data);
	}

	if(freenode)
		htmlnode_free(rootnode);
}

gboolean htmlnode_to_plane_text(HtmlNode * rootnode, htmlnode_appender appender, gpointer user_data )
{
	return htmlnode_to_plane_text_internal(rootnode,appender,user_data,0,FALSE);
}

gboolean htmlnode_to_plane_text_and_free(HtmlNode * rootnode, htmlnode_appender appender , gpointer user_data )
{
	return htmlnode_to_plane_text_internal(rootnode,appender,user_data,0,TRUE);
}


void htmlnode_free(HtmlNode * rootnode)
{
	GList * children = rootnode->children;

	rootnode->children = NULL;

	g_free((gpointer)rootnode->tag);
	g_free((gpointer)rootnode->plane);

	g_list_foreach(rootnode->attr,(GFunc)g_free,NULL);
	g_list_free(rootnode->attr);
	g_list_foreach(children,(GFunc)htmlnode_free,NULL);
	g_list_free(rootnode->children);

	if(rootnode->parent && rootnode->parent->children )
		rootnode->parent->children = g_list_remove(rootnode->parent->children,rootnode);
	g_free(rootnode);
}
