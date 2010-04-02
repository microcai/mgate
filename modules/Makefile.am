#AUTOMAKE_OPTIONS = foreign

libexecdir=${prefix}/libexec/monitor/modules

AM_CPPFLAGS = @GLIB2_CFLAGS@ -I$(top_srcdir)/src -fpic

LIBS = -lpthread -shared $(LDFLAGS)

libexec_PROGRAMS =  kpolice.so  kurl.so kqq.so kmsn.so
if ENABLE_HOTEL

libexec_PROGRAMS +=   klz.so kjincheng.so
AM_CPPFLAGS += -DENABLE_HOTEL 

endif

if ENABLE_HOTEL

klz_so_SOURCES = klz.cpp
klz_so_DEPENDENIES= libksql.so

kjincheng_so_SOURCES = kServer.cpp kServer.hpp

endif

kmsn_so_SOURCES = kmsn.cpp

kqq_so_SOURCES = kqq.cpp

kpolice_so_SOURCES = kpolice.cpp kpolice.h

kurl_so_SOURCES = kurl.cpp


