/*
 * htmlnote.h
 *
 *  Created on: 2010-4-25
 *      Author: cai
 */

#ifndef HTMLNOTE_H_
#define HTMLNOTE_H_

#include <glib.h>

typedef struct _HtmlNode{
	struct _HtmlNode * parent; //parent
	GList * children; //children
    const char		*	tag; // < > </> the tag
    const char		* 	plane; // not a tag ,but plane text
    GList* attr; // attrs
}HtmlNode;

typedef void (*htmlnode_appender)(const char * , gpointer);

G_BEGIN_DECLS

HtmlNode * htmlnode_append_child(HtmlNode * node,HtmlNode * child);
HtmlNode * htmlnode_append_attr(HtmlNode * node, const char * attr);
HtmlNode * htmlnode_append_attr_take(HtmlNode * node, char * attr);

HtmlNode * htmlnode_new(HtmlNode * parent, const char * tag , ...);
HtmlNode * htmlnode_new_text(HtmlNode * parent, const char * text );
HtmlNode * htmlnode_newv(HtmlNode * parent, const char * tag, va_list v);
HtmlNode * htmlnode_new_table(HtmlNode * parent,const char * attrlist, ...);

//释放一棵树
void htmlnode_free(HtmlNode * rootnode);
//用这个把 HTML 树展开，:)
gboolean htmlnode_to_plane_text(HtmlNode * rootnode, htmlnode_appender , gpointer user_data );

//同时 free
gboolean htmlnode_to_plane_text_and_free(HtmlNode * rootnode, htmlnode_appender appender , gpointer user_data );

void htmlnode_free(HtmlNode * rootnode);
G_END_DECLS

#endif /* HTMLNOTE_H_ */
