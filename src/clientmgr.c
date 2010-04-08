/*
 * clientmgr.c
 *
 *  Created on: 2010-4-8
 *      Author: cai
 */

#include <sys/syslog.h>
#include <sys/socket.h>
#include <net/ethernet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <pcap.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <pcap/pcap.h>
#include <syslog.h>
#include <string.h>
#include <glib.h>

#ifdef HAVE_GETTEXT
#include <locale.h>
#include <libintl.h>
#define _(x) gettext(x)
#define N_(x) (x)
#endif

#include "clientmgr.h"

enum{
	CLIENT_NAME = 3, //名字
	CLIENT_ID, //身份证
	CLIENT_IP, // ip 地址
	CLIENT_MAC

};

static void client_set_property(GObject *object, guint property_id,const GValue *value, GParamSpec *pspec);
static void client_get_property(GObject *object, guint property_id,GValue *value, GParamSpec *pspec);

static void client_class_init(ClientClass * klass)
{
	GObjectClass * gobjclass = G_OBJECT_CLASS(klass);
	gobjclass->set_property = client_set_property;
	gobjclass->get_property = client_get_property;

	g_object_class_install_property(gobjclass,CLIENT_NAME,
			g_param_spec_string("name","name","name","",G_PARAM_CONSTRUCT_ONLY|G_PARAM_READABLE));
	g_object_class_install_property(gobjclass,CLIENT_ID,
			g_param_spec_string("id","id","id","N/A",G_PARAM_CONSTRUCT_ONLY|G_PARAM_READABLE));
	g_object_class_install_property(gobjclass,CLIENT_IP,
			g_param_spec_int("ip","ip","ip",INADDR_ANY,INADDR_NONE,INADDR_NONE,G_PARAM_CONSTRUCT|G_PARAM_READWRITE));
//	g_object_class_install_property(gobjclass,CLIENT_NAME,
//			g_param_spec_int("ip","ip","ip","",G_PARAM_CONSTRUCT|G_PARAM_READWRITE));


}

static void client_init(Client * obj)
{
	obj->name = g_string_new("");
	obj->id = g_string_new("");
	obj->ip = INADDR_NONE;
}

static void client_set_property(GObject *object, guint property_id,const GValue *value, GParamSpec *pspec)
{
	Client * obj;

	switch (property_id)
	{
	case CLIENT_NAME:
		obj->name = g_string_assign(obj->name,g_value_get_string(value));

		break;
	case CLIENT_ID:
		obj->id = g_string_assign(obj->id,g_value_get_string(value));
		break;
	case CLIENT_IP:
		obj->ip = g_value_get_int(value);
	default:
		break;
	}

}

static void client_get_property(GObject *object, guint property_id,GValue *value, GParamSpec *pspec)
{
	Client * obj;
	switch (property_id)
	{
	case CLIENT_NAME:
		g_value_set_string(value,obj->name->str);
		break;
	case CLIENT_ID:
		g_value_set_string(value,obj->id->str);
		break;
	case CLIENT_IP:
		g_value_set_int(value,obj->ip);
		break;
	default:
		break;
	}
}

G_DEFINE_TYPE(Client,client,G_TYPE_OBJECT);

Client * clientmgr_get_client_by_mac()
{

}

void clientmgr_init()
{
	g_type_init();
}
