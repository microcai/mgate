/*
 * htmlnode.c - tools to generate html file
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

/**
 * SECTION:htmlnode
 * @short_description: html 标记转化
 * @title:HtmlNode
 * @stability: Stable
 * @include: htmlnode.h
 *
 * htmlnode 表达了 html 页面的一个标签。 标签可以嵌套，htmlnode 通过连接子节点组织树形结
 * 构来描述 html 页面。
 *
 * htmlnode 提供了对 html 输出的优雅的无错误的方式。
 * 首先通过 html_node_new(NULL,"html",NULL) 构造 html 总页面
 * 然后用已经存在的节点为父节点不断通过调用 html_node_new() 来嵌套 html 标记
 * 当然，已有的无父 html 节点可以通过 htmlnode_append_child() 被加入到一个
 * html节点/标记 中
 *
 * 构造好html页面后，就可以通过调用 htmlnode_to_plane_text() 来开始传化为 html 页面了
 * htmlnode_to_plane_text() 一次只转化一小部分，每次完成一小部分后都会调用 @appender 参数
 * 制定的类型为 #htmlnode_appender 的回调函数，并以转化出的字符串为参数。
 *
 */
#include <stdarg.h>
#include <string.h>
#include <glib.h>
#include "htmlnode.h"

HtmlNode * htmlnode_new(HtmlNode * parent, const char * tag , ...)
{
	va_list v;

	HtmlNode * node = g_slice_new0(HtmlNode);

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

HtmlNode * htmlnode_newv(HtmlNode * parent, const char * tag, const char * attrlist, va_list v)
{

	HtmlNode * node = g_slice_new0(HtmlNode);


//	va_copy(v,*vv);

	node->parent = parent;

	if(parent)
		htmlnode_append_child(parent,node);

	if(tag)
	{
		node->tag = g_strdup(tag);
		if (attrlist)
		{
			htmlnode_append_attr(node, attrlist);

			while ((tag = va_arg(v,char*)))
			{
				htmlnode_append_attr(node, tag);
			}
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

HtmlNode * htmlnode_new_text_printf(HtmlNode * parent, const char * fmt,...)
{
	va_list va;
	va_start(va,fmt);
	gchar * txt = g_strdup_vprintf(fmt,va);
	va_end(va);
	HtmlNode * ret = htmlnode_new_text(parent,txt);
	g_free(txt);
	return ret;
}

HtmlNode * htmlnode_new_head(HtmlNode * parent, const char * attrlist, ...)
{
	va_list v;
	va_start(v,attrlist);
	return htmlnode_newv(parent,"head",attrlist,v);
}

HtmlNode * htmlnode_new_body(HtmlNode * parent, const char * attrlist, ...)
{
	va_list v;
	va_start(v,attrlist);
	return htmlnode_newv(parent,"body",attrlist,v);
}

HtmlNode * htmlnode_new_table(HtmlNode * parent,const char * attrlist, ...)
{
	va_list v;
	va_start(v,attrlist);
	return htmlnode_newv(parent,"table",attrlist,v);
}

HtmlNode * htmlnode_new_form(HtmlNode * parent,const char * method , const char * action ,const char * attrlist, ...)
{
	va_list v;

	method = g_strdup_printf("method=\"%s\"",method);

	action = g_strdup_printf("action=\"%s\"",action);

	HtmlNode * node = htmlnode_new(parent,"form",method,action,NULL);

	g_free((gpointer)method);
	g_free((gpointer)action);

	va_start(v,attrlist);

	while (attrlist)
	{
		htmlnode_append_attr(node,(const char *)attrlist);
		attrlist = va_arg(v,char*);
	}
	va_end(v);
	return node;
}

HtmlNode * htmlnode_new_iframe(HtmlNode * parent, const char * src,const char * attrlist, ...)
{
	va_list v;

	src = g_strdup_printf("src=\"%s\"",src);

	HtmlNode * node = htmlnode_new(parent,"iframe",src,NULL);

	g_free((gpointer)src);


	va_start(v,attrlist);

	while (attrlist)
	{
		htmlnode_append_attr(node,(const char *)attrlist);
		attrlist = va_arg(v,char*);
	}
	va_end(v);
	return node;
}

HtmlNode * htmlnode_new_js(HtmlNode * parent,const char * jsfunc)
{
	HtmlNode *js = htmlnode_new(parent,"script","language=\"text/javascript\"",0);
	htmlnode_new_text_printf(js,"\n<!--\n\t%s\n//-->\n",jsfunc);
	return js;
}

HtmlNode * htmlnode_new_jssrc(HtmlNode * parent,const char * jssrc)
{
	char * jssrcurl = g_strdup_printf("src=\"%s\"",jssrc);
	HtmlNode *js = htmlnode_new(parent,"script","language=\"text/javascript\"",jssrcurl,0);
	g_free(jssrcurl);
	return js;
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

static gboolean htmlnode_to_plane_text_internal(HtmlNode * rootnode, htmlnode_appender appender, gpointer user_data , int depth);

static void appendlist(char * attrstr, gpointer user_data)
{
	struct appendliststruct{
		gpointer	user_data;
		htmlnode_appender appender;
	}*appendlist_data = user_data;

	appendlist_data->appender(" ", appendlist_data->user_data);
	appendlist_data->appender(attrstr, appendlist_data->user_data);
}

static void node_for(HtmlNode * node, gpointer user_data)
{
	struct {
		gpointer	user_data;
		htmlnode_appender appender;
		gboolean * first_call;
		gboolean * newline_close;
		int depth;
	}*node_for_data = user_data;

	if(*(node_for_data->first_call))
	{
		*(node_for_data->first_call) = FALSE;
		if (node->tag)
		{
			node_for_data->appender("\n", node_for_data->user_data);
			*(node_for_data->newline_close) = TRUE;
		}
	}
	htmlnode_to_plane_text_internal(node, node_for_data->appender, node_for_data->user_data, node_for_data->depth + 1);
}

static gboolean htmlnode_to_plane_text_internal(HtmlNode * rootnode, htmlnode_appender appender, gpointer user_data , int depth)
{
	gchar * kg = NULL ;

	kg = g_malloc0(depth + 1);
	memset(kg, ' ', depth);

	if (rootnode->tag)
	{
		if (depth)
		{
			appender(kg, user_data);
		}

		appender("<", user_data);

		appender(rootnode->tag, user_data);

		if (rootnode->attr)
		{
			struct {
				gpointer	user_data;
				htmlnode_appender appender;
			}appendlist_data;
			appendlist_data.user_data = user_data;
			appendlist_data.appender = appender;

			g_list_foreach(rootnode->attr, (GFunc) appendlist, &appendlist_data);
		}

		appender(">", user_data);

		gboolean first_call =TRUE ;
		gboolean newline_close = FALSE;

		if (rootnode->children)
		{
			struct {
				gpointer	user_data;
				htmlnode_appender appender;
				gboolean * first_call;
				gboolean * newline_close;
				int depth;
			}node_for_data;
			node_for_data.user_data = user_data;
			node_for_data.appender = appender;

			node_for_data.first_call = & first_call;
			node_for_data.newline_close = & newline_close;
			node_for_data.depth = depth;

			g_list_foreach(rootnode->children, (GFunc) node_for, &node_for_data);
		}

		if (newline_close && depth && !first_call)
		{
			appender(kg, user_data);
		}

		appender("</", user_data);
		appender(rootnode->tag, user_data);
		appender(">\n", user_data);
	}
	else
	{
		appender(rootnode->plane, user_data);
	}

	g_free(kg);

	return TRUE;
}
/**
 * htmlnode_to_plane_text:
 * @rootnode : 根 html 节点
 * @appender : 回调函数，参见 #htmlnode_appender
 * @user_data : 传递给 @appender 的参数
 *
 * 用这个把 HTML 树展开，:)
 *
 * Returns: TRUE 成功， FALSE 失败
 */
gboolean htmlnode_to_plane_text(HtmlNode * rootnode, htmlnode_appender appender, gpointer user_data )
{
	return htmlnode_to_plane_text_internal(rootnode,appender,user_data,0);
}

/**
 * htmlnode_to_plane_text_and_free:
 * @rootnode : 根 html 节点
 * @appender : 回调函数，参见 #htmlnode_appender
 * @user_data : 传递给 @appender 的参数
 *
 * 用这个把 HTML 树展开，:)，同时释放html树
 *
 * Returns: TRUE 成功， FALSE 失败
 */
gboolean htmlnode_to_plane_text_and_free(HtmlNode * rootnode, htmlnode_appender appender , gpointer user_data )
{
	gboolean ret = htmlnode_to_plane_text_internal(rootnode,appender,user_data,0);
	htmlnode_free(rootnode);
	return ret;
}

/**
 * htmlnode_free:
 * @rootnode: html 根节点
 *
 * 释放 html 树
 */
void htmlnode_free(HtmlNode * rootnode)
{
	GList * children = rootnode->children;

	rootnode->children = NULL;

	g_free((gpointer)rootnode->tag);
	g_free((gpointer)rootnode->plane);

	g_list_foreach(rootnode->attr,(GFunc)g_free,NULL);
	g_list_free(rootnode->attr);

	if(rootnode->parent && rootnode->parent->children )
		rootnode->parent->children = g_list_remove(rootnode->parent->children,rootnode);

	g_list_foreach(children,(GFunc)htmlnode_free,NULL);
	g_list_free(children);

	g_slice_free(HtmlNode,rootnode);
}
