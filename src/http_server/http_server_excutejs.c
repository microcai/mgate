
/*
 * http_server_excutejs.c  --
 * 	这个文件用来执行js文件向用户返回内容
 *
 *      Copyright 2011 薇菜工作室
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
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <unistd.h>
#include <sys/resource.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ucontext.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <libsoup/soup.h>
#include <jsapi.h>

#include "utils.h"
#include "http_server.h"
#include "global.h"
#include "htmlnode.h"
#include "html_paths.h"

#define MAX_BYTES (256*1024*1024)
#define STACK_SIZE	(10*1024*1024)

static JSRuntime * jsrt;

static JSClass global_class = {
    "global",JSCLASS_GLOBAL_FLAGS,
    JS_PropertyStub,
    JS_PropertyStub,
    JS_PropertyStub,
    JS_StrictPropertyStub,
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub,
    JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

void SoupServer_excutejs(SoupServer *server, SoupMessage *msg,
		const char *path, GHashTable *query, SoupClientContext *client,
		gpointer user_data)
{

	//初始化 js 引擎
	if(!jsrt)
		jsrt = JS_NewRuntime(MAX_BYTES);

    JSObject *global;

	//创建 JS 上下文
	JSContext * jscx = JS_NewContext(jsrt,getpagesize()*1024);

    global = JS_NewCompartmentAndGlobalObject(jscx, &global_class,NULL);

    JS_InitStandardClasses(jscx, global);

    JSBool
    jstest(JSContext *cx,uintN argc, jsval *rval)
    {
    	g_debug("js called me!");

//    	*rval = (jsval) JS_NewStringCopyZ(cx,"nihao");
    	JS_SET_RVAL(cx,rval,STRING_TO_JSVAL(JS_NewStringCopyZ(cx,"nihao")));
    	return TRUE;
    }

    JSFunctionSpec jsfunctions[] = {
    		{"test", (JSNative)jstest, 0, 0 },
    		{0,0,0,0},
    };

    JS_DefineFunctions(jscx, global, jsfunctions);

	//创建独立的用户空间线程

    char * script = " test();  ";

    JSScript *  js = JS_CompileScript(jscx,global,script,strlen(script),0,0);

	jsval	jsv;

	JS_ExecuteScript(jscx,global,js,&jsv);

	JSString * jsresult =  JS_ValueToString(jscx,jsv);

	size_t jsstrlen;
	const jschar * result16 = JS_GetStringCharsZAndLength(jscx,jsresult,&jsstrlen);

	char * result = g_utf16_to_utf8(result16,jsstrlen,0,0,0);

	soup_message_set_status(msg,SOUP_STATUS_OK);

	soup_message_set_response(msg,"text/html",SOUP_MEMORY_TAKE,result,strlen(result));

	JS_DestroyScript(jscx,js);
	JS_DestroyContext(jscx);

}
