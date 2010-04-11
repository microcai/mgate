AUTOMAKE_OPTIONS = foreign

AM_CPPFLAGS = @GLIB2_CFLAGS@ @MYSQL_CFLAGS@

LIBS = @MYSQL_LIBS@ @GLIB2_LIBS@ -lpcap -ldl

sbindir=/etc/init.d/

bin_PROGRAMS = monitor 
bin_SCRIPTS = runmonitor
sbin_SCRIPTS = monitord

EXTRA_DIST = runmonitor monitord

md5_SOURCE = md5.c md5.h

monitor_SOURCES =main.c global.h

monitor_SOURCES += pcap_thread.c  pcap_thread.h 
monitor_SOURCES += pcap_hander.c pcap_hander.h
monitor_SOURCES += clientmgr.c clientmgr.h
monitor_SOURCES += utils.c utils.h
monitor_SOURCES += module.c module.h
monitor_SOURCES += $(md5_SOURCE)
monitor_SOURCES += kpolice.c kpolice.h
monitor_SOURCES += ksql.h

		 
if WITH_MYSQL
  monitor_SOURCES += ksql.c ksql_static_template.h
endif
