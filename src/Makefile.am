AUTOMAKE_OPTIONS = foreign

DIST_SUBDIRS = sqlclient http_server

SUBDIRS = sqlclient @http_server@

AM_CPPFLAGS = @GLIB2_CFLAGS@ -I$(srcdir)/sqlclient

LIBS += @NEED_ALL_LIBS@ @MYSQL_LIBS@ @SQLITE3_LIBS@ \
			$(builddir)/sqlclient/libsqlclient.a \
			@http_server_lib@ 

DISTCLEANFILES = monitord

sbindir=/etc/init.d/

bin_PROGRAMS = monitor 
sbin_SCRIPTS = monitord

EXTRA_DIST =  monitord.in

md5_SOURCE = misc/md5.c misc/md5.h

monitor_SOURCES =main.c global.h i18n.h

monitor_SOURCES += pcap_thread.c  pcap_thread.h 
monitor_SOURCES += pcap_hander.c pcap_hander.h
monitor_SOURCES += clientmgr.c clientmgr.h
monitor_SOURCES += utils.c utils.h
monitor_SOURCES += module.c module.h
monitor_SOURCES += $(md5_SOURCE)
monitor_SOURCES += kpolice.c kpolice.h
monitor_SOURCES += g_socket_client_proxy.c g_socket_client_proxy.h

monitor_SOURCES += ksql.h ksql.c
monitor_SOURCES += traffic_status.c traffic_status.h


monitor_DEPENDENCIES = @http_server_lib@  $(builddir)/sqlclient/libsqlclient.a

## SMS
monitor_CFLAGS = -I$(srcdir)/sms

monitor_SOURCES += sms/at_modem.h sms/at_modem.c
monitor_SOURCES += sms/smsapi.c sms/smsapi.h
monitor_SOURCES += sms/sms.c sms/sms.h
monitor_SOURCES += sms/smsthread.c sms/smsthread.h
