#
#  Copyright 2009 microcai <microcai@sina.com>
# 
#  See COPYING for more details about this software's license
# 

AM_CFLAGS = -fPIC
AM_CXXFLAGS = -fPIC @GLIB2_CFLAGS@

libexecdir=${prefix}/libexec/monitor/modules

libexec_PROGRAMS = libdreamtop.so

libdreamtop_so_LDADD = -ldl $(LD_MYSQL_FLAGS) @GLIB2_LIBS@
libdreamtop_so_LDFLAGS = -shared

libdreamtop_so_SOURCES = libmicrocai.cpp md5.c md5.h \
						 libdreamtop.h functions.h \
						 libmicrocai-macros.h libmicrocai-queue.h \
						 libmicrocai-types.h \
						 load_module.cpp ksql.h ksql.cpp \
						 protocol_handler.cpp 
						 
if ENABLE_HOTEL
libdreamtop_so_SOURCES += clients.cpp http_redirect.cpp
#只有hotel才要http redirect ， 才会用到 libnet 
libdreamtop_so_LDADD += -lnet	
endif 

if WITH_MYSQL
#libdreamtop_so_SOURCES += KSQL/kmysql.cpp KSQL/kmysql.h
endif

if WITH_SQLITE3
libdreamtop_so_LDADD += -lsqlite3
libdreamtop_so_SOURCES += KSQL/ksqlite.h KSQL/ksqlite.cpp
endif


