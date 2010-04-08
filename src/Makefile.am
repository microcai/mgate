AUTOMAKE_OPTIONS = foreign

AM_CPPFLAGS = @GLIB2_CFLAGS@

sbindir=/etc/init.d/

bin_PROGRAMS = monitor 
bin_SCRIPTS = runmonitor
sbin_SCRIPTS = monitord

EXTRA_DIST = runmonitor monitord


monitor_SOURCES = ip_filter.c main.c auto_update.cpp protocol_def.h \
                      libdreamtop.c md5.c md5.h \
						 libdreamtop.h functions.h \
						 libmicrocai-macros.h libmicrocai-queue.h \
						 libmicrocai-types.h \
						 load_module.cpp ksql.h ksql.cpp \
						 protocol_handler.c
						 
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

 
