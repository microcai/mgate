AUTOMAKE_OPTIONS = foreign

AM_CPPFLAGS = @NEED_ALL_CFLAGS@ @MYSQL_CFLAGS@

LIBS += @NEED_ALL_LIBS@ @MYSQL_LIBS@

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
monitor_SOURCES += gsqlconnect.c  gsqlconnect.h
monitor_SOURCES += gsqlresult.c gsqlresult.h

monitor_SOURCES += ksql.h ksql.c
monitor_SOURCES += http_server.c http_server.h
monitor_SOURCES += htmlnode.c htmlnode.h

monitor_LDADD =  favicon.o
monitor_DEPENDENCES = favicon.ico
EXTRA_monitor_SOURCES = favicon.ico

favicon.o: favicon.ico
	$(OBJCOPY)	-I binary -B @obj_machine@ -O @obj_format@ $< $@
	
## SMS
monitor_CFLAGS = -I$(srcdir)/sms

monitor_SOURCES += sms/at_modem.h sms/at_modem.c
monitor_SOURCES += sms/smsapi.c sms/smsapi.h
monitor_SOURCES += sms/sms.c sms/sms.h
monitor_SOURCES += sms/smsthread.c sms/smsthread.h

		 
if WITH_MYSQL
  monitor_SOURCES += gsqlconnect_mysql.c	 gsqlconnect_mysql.h
  monitor_SOURCES += ksql_static_template.h
  AM_CPPFLAGS += -D__HAVE_MYSQL
endif
