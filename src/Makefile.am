AUTOMAKE_OPTIONS = foreign

DIST_SUBDIRS = sqlclient http_server

SUBDIRS = sqlclient @http_server@

AM_CPPFLAGS = @GLIB2_CFLAGS@ -I$(srcdir)/sqlclient

LIBS += @NEED_ALL_LIBS@ @MYSQL_LIBS@ @SQLITE3_LIBS@ \
			$(builddir)/sqlclient/libsqlclient.a \
			@http_server_lib@ 

DISTCLEANFILES = monitord

OBJCOPY=@OBJCOPY@ 

sbindir=/etc/init.d/

bin_PROGRAMS = monitor 
sbin_SCRIPTS = monitord

EXTRA_DIST =  monitord.in

md5_SOURCE = md5.c md5.h

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

monitor_SOURCES += sqlclient/gsqlconnect.c  sqlclient/gsqlconnect.h
monitor_SOURCES += sqlclient/gsqlresult.c sqlclient/gsqlresult.h

monitor_LDADD =  favicon.o 

EXTRA_monitor_SOURCES = favicon.ico

monitor_DEPENDENCIES = favicon.o @http_server_lib@  $(builddir)/sqlclient/libsqlclient.a

favicon.o: favicon.ico
	$(OBJCOPY)	-I binary -B @obj_machine@ -O @obj_format@ $< $@
	
## SMS
monitor_CFLAGS = -I$(srcdir)/sms

monitor_SOURCES += sms/at_modem.h sms/at_modem.c
monitor_SOURCES += sms/smsapi.c sms/smsapi.h
monitor_SOURCES += sms/sms.c sms/sms.h
monitor_SOURCES += sms/smsthread.c sms/smsthread.h
