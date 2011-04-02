
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
#include <sys/mman.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <libsoup/soup.h>
#include <jsapi.h>

#include "utils.h"
#include "http_server.h"
#include "global.h"
#include "htmlnode.h"
#include "html_paths.h"

typedef	struct	uthread{
	ucontext_t  oucp;
	ucontext_t	 ucp;
	volatile int			 need_exit;
}uthread;


#define MAX_BYTES (256*1024*1024)
#define STACK_SIZE	(10*1024*1024)
#define BODY500		" internal server error! "


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


static  JSBool yield(JSContext *cx,uintN argc, jsval *rval)
{
	uthread * thread = JS_GetContextPrivate(cx);

	swapcontext(&thread->ucp,&thread->oucp);

	return TRUE;
}

static JSFunctionSpec jsfunctions[] = {
		{"yield", (JSNative)yield, 0, 0 },
		{0,0,0,0},
};


static void exec_ucontext(SoupServer *server, SoupMessage *msg,
		const char *path, GHashTable *query, SoupClientContext *client,
		gpointer user_data,uthread * thread)
{
	gpointer filecontent;
	gsize filelength;
	const zipRecord * ziprc;

	JSScript *  js;
	JSObject *global;

	//创建 JS 上下文
	JSContext * jscx = JS_NewContext(jsrt,getpagesize()*1024);

	JS_SetContextPrivate(jscx,thread);

    global = JS_NewCompartmentAndGlobalObject(jscx, &global_class,NULL);

    JS_InitStandardClasses(jscx, global);

	JS_DefineFunctions(jscx, global, jsfunctions);


    //看是否存在对应文件
    //没有就到 ZIP 压缩资源里找
	if (overlay_get_file(path, user_data, &filecontent, &filelength))
	{
	    js = JS_CompileScript(jscx,global,filecontent,filelength,0,0);
	    munmap(filecontent,filelength);

	}else if( (ziprc = static_file_get_zip(path)))
	{
		//解压
		filelength = ziprc->size_orig;

		filecontent = g_malloc0(filelength);

		unzip_buffer(filecontent,&filelength,ziprc);

		//有的
	    js = JS_CompileScript(jscx,global,filecontent,filelength,0,0);
	    g_free(filecontent);
	}else
	{
		soup_server_unpause_message(server,msg);
		return SoupServer_path_404(server,msg,path,query,client,user_data);
	}

	if(!js){
		// 500 , internal server error
		soup_message_set_status(msg,SOUP_STATUS_INTERNAL_SERVER_ERROR);
		soup_message_set_response(msg,"text/html",SOUP_MEMORY_STATIC,BODY500,sizeof(BODY500)-1);
		goto do_exit ;
	}


	jsval	jsv;

	JS_ExecuteScript(jscx,global,js,&jsv);

	JSString * jsresult =  JS_ValueToString(jscx,jsv);

	size_t jsstrlen;
	const jschar * result16 = JS_GetStringCharsZAndLength(jscx,jsresult,&jsstrlen);

	char * result = g_utf16_to_utf8(result16,jsstrlen,0,0,0);

	soup_message_set_status(msg,SOUP_STATUS_OK);

	soup_message_set_response(msg,"text/html",SOUP_MEMORY_TAKE,result,strlen(result));

	soup_server_unpause_message(server,msg);

	JS_DestroyScript(jscx,js);
	JS_DestroyContext(jscx);

do_exit:
	thread->need_exit = TRUE;
	soup_server_unpause_message(server,msg);
	swapcontext(&thread->ucp,&thread->oucp);
}

static gboolean schedule(uthread * thread)
{
	if(!thread->need_exit){
		swapcontext(&thread->oucp,&thread->ucp);
		return TRUE;
	}

	g_free(thread->ucp.uc_stack.ss_sp);
	g_free(thread);
	return FALSE;
}

void SoupServer_excutejs(SoupServer *server, SoupMessage *msg,
		const char *path, GHashTable *query, SoupClientContext *client,
		gpointer user_data)
{
	//初始化 js 引擎
	if(!jsrt)
		jsrt = JS_NewRuntime(MAX_BYTES);

	soup_server_pause_message(server,msg);
	uthread* 	thread = g_new0(uthread,1);

	getcontext(& thread->ucp);
	//创建独立的用户空间线程
	thread->ucp.uc_stack.ss_size = 1024*1024;
	thread->ucp.uc_stack.ss_sp  = g_malloc0(thread->ucp.uc_stack.ss_size);

	makecontext(&thread->ucp, (gpointer) exec_ucontext,8,server,msg,path,query,client,user_data,&thread->oucp,&thread->ucp);

	g_idle_add_full(G_PRIORITY_LOW,(GSourceFunc)schedule,thread,NULL);

	return ;
}
