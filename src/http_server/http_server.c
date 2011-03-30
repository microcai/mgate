/*
 * http_server.c -- HTTP 服务器实现
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
#include "clientmgr.h"
#include "utils.h"
#include "http_server.h"
#include "global.h"
#include "htmlnode.h"
#include "html_paths.h"
#include "smsserver_connector.h"

#define MAX_BYTES (256*1024*1024)
#define STACK_SIZE	(10*1024*1024)

#ifdef MOZJS
#include "jsapi.h"
#endif

static void SoupServer_path_root(SoupServer *server, SoupMessage *msg,
		const char *path, GHashTable *query, SoupClientContext *client,
		gpointer user_data);

static SoupServer * server;

int start_server()
{
	GError * err = NULL;

	g_assert(gkeyfile);

	int port = g_key_file_get_integer(gkeyfile,"http","port",&err);
	if(err)
	{
		port = 8000;
		g_error_free(err);
		g_warning(_("using port %d as default duto the missing [http]:[port] settings"),port);
	}

	while (!(server = soup_server_new(SOUP_SERVER_ASYNC_CONTEXT,
			g_main_context_get_thread_default(), "port", port,
			SOUP_SERVER_SERVER_HEADER, PACKAGE_STRING " simple http server", NULL)))
	{
		g_warning(_("server failt to start at port %d, will use random port!"),port);
		port = 0;
	}

	//初始化 js 引擎
//	JSRuntime * jsrt = JS_NewRuntime(MAX_BYTES);
//	JSContext * jsct = JS_NewContext(jsrt,STACK_SIZE);


	smsserver_pinger_start();

	g_debug(_("server started at port %u"),soup_server_get_port(server));

	soup_server_add_handler(server,"/login",SoupServer_path_login,NULL,NULL);

	soup_server_add_handler(server,"/info",SoupServer_path_info,NULL,NULL);

	soup_server_add_handler(server,"/log",SoupServer_path_log,NULL,NULL);

	soup_server_add_handler(server,"/favicon.ico",SoupServer_path_static_file,NULL,NULL);

	soup_server_add_handler(server,"/",SoupServer_path_root,NULL,NULL);

	soup_server_add_handler(server,"/keep_alive",SoupServer_path_keep_alive,NULL,NULL);

	soup_server_add_handler(server,"/getcode.html",SoupServer_path_getverifycode,NULL,NULL);

	soup_server_run_async(server);

	g_timeout_add_seconds(1,celect_usage,NULL);

	g_timeout_add_seconds(16,remove_outdated_inactive_client,GINT_TO_POINTER(30));

	return soup_server_get_port(server);
}

static void SoupServer_path_root(SoupServer *server, SoupMessage *msg,
		const char *path, GHashTable *query, SoupClientContext *client,
		gpointer user_data)
{
	if(g_strcmp0(path,"/")==0)
	{
		return SoupServer_path_static_file(server,msg,"/index.html",query,client,user_data);
	}

	return SoupServer_path_static_file(server,msg,path,query,client,user_data);


	// 察看是不是一个 js 脚本

	//是的话就进入 js 脚本模式。 js 脚本是给外部开发者用的。所以是调用的文件，而不是内置资源

    JSObject *global;
    JSClass global_class = {
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

	//创建 JS 上下文
	JSRuntime * jsrt = JS_NewRuntime(4096*getpagesize());
	JSContext * jscx = JS_NewContext(jsrt,getpagesize()*1024);
//	JS_NewObject(jscx,)

//	JS_InitCTypesClass();

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

//	JS_free(jscx,jsresult);

	soup_message_set_status(msg,SOUP_STATUS_OK);

	soup_message_set_response(msg,"text/html",SOUP_MEMORY_TAKE,result,strlen(result));

	JS_DestroyScript(jscx,js);
	JS_DestroyContext(jscx);
	JS_DestroyRuntime(jsrt);

	return ;
	//fall back
}

void soup_message_body_appender(const gchar * txt, SoupMessageBody * body)
{
	soup_message_body_append(body,SOUP_MEMORY_COPY,txt,strlen(txt));
}
