#AUTOMAKE_OPTIONS = foreign

libexecdir=${libdir}/@PACKAGE_NAME@/modules

AM_CPPFLAGS = @GLIB2_CFLAGS@ -I$(top_srcdir)/src -fpic

LIBS = @GLIB2_LIBS@ -shared

EXTRA_PROGRAMS= libkqq.so libkurl.so libkmsn.so libredirect.so klz.so kjincheng.so 

libexec_PROGRAMS = $(NULL)

if WITH_KQQ
libexec_PROGRAMS += libkqq.so
endif

if WITH_KURL
libexec_PROGRAMS += libkurl.so
endif

if WITH_KMSN
libexec_PROGRAMS += libkmsn.so
endif

if BUILD_HTTPREDIRECT
libexec_PROGRAMS += libredirect.so
endif


libredirect_so_SOURCES = http_redirect.c
libredirect_so_LDADD = -lnet

libkqq_so_SOURCES = kqq.c
libkurl_so_SOURCES = kurl.cpp
libkmsn_so_SOURCES = kmsn.cpp

#libexec_PROGRAMS +=   klz.so kjincheng.so
#AM_CPPFLAGS += -DENABLE_HOTEL 

klz_so_SOURCES = klz.cpp

kjincheng_so_SOURCES = kServer.cpp kServer.hpp





