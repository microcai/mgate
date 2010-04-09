AUTOMAKE_OPTIONS = foreign

AM_CPPFLAGS = @GLIB2_CFLAGS@

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

monitor_SOURCES += $(md5_SOURCE)

monitor_SOURCES += auto_update.cpp libdreamtop.c libdreamtop.h functions.h \
						 libmicrocai-macros.h libmicrocai-queue.h \
						 libmicrocai-types.h \
						 load_module.cpp ksql.h ksql.cpp
						 
monitor_LDADD = @GLIB2_LIBS@ -lpcap -ldl 
monitor_LDFLAGS = -Wl,-rpath,${prefix}/libexec/monitor/modules

					
if ENABLE_HOTEL
monitor_SOURCES += clients.cpp http_redirect.c 
#只有hotel才要http redirect ， 才会用到 libnet 
monitor_LDADD += -lnet	

monitor_CXXFLAGS  = -DENABLE_HOTEL
endif


if WITH_MYSQL
 # monitor_SOURCES += KSQL/kmysql.cpp KSQL/kmysql.h
 # monitor_LDADD +=$(LD_MYSQL_FLAGS)
endif

if WITH_SQLITE3
monitor_LDADD += -lsqlite3
monitor_SOURCES += KSQL/ksqlite.h KSQL/ksqlite.cpp
endif

 
