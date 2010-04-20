AUTOMAKE_OPTIONS = foreign

AM_CPPFLAGS = @NEED_ALL_CFLAGS@ @MYSQL_CFLAGS@

LIBS += @NEED_ALL_LIBS@ @MYSQL_LIBS@ 

sbindir=/etc/init.d/

bin_PROGRAMS = monitor 
sbin_SCRIPTS = monitord

EXTRA_DIST = runmonitor monitord

md5_SOURCE = md5.c md5.h

monitor_SOURCES =main.c global.h i18n.h

monitor_SOURCES += pcap_thread.c  pcap_thread.h 
monitor_SOURCES += pcap_hander.c pcap_hander.h
monitor_SOURCES += clientmgr.c clientmgr.h
monitor_SOURCES += utils.c utils.h
monitor_SOURCES += module.c module.h
monitor_SOURCES += $(md5_SOURCE)
monitor_SOURCES += kpolice.c kpolice.h
monitor_SOURCES += gsqlconnect.c  gsqlconnect.h
monitor_SOURCES += gsqlresult.c gsqlresult.h

monitor_SOURCES += ksql.h ksql.c
monitor_SOURCES += http_server.c

		 
if WITH_MYSQL
  monitor_SOURCES += gsqlconnect_mysql.c	 gsqlconnect_mysql.h
  monitor_SOURCES += ksql_static_template.h
endif
