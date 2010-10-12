#AUTOMAKE_OPTIONS = foreign

libexecdir=${libdir}/@PACKAGE_NAME@/modules

AM_CPPFLAGS = @GLIB2_CFLAGS@ -I$(top_srcdir)/src -fpic

LIBS += @GLIB2_LIBS@ -shared



libexec_PROGRAMS = $(NULL)

if WITH_KQQ
libexec_PROGRAMS += libkqq.so
#  libkmsn.so
endif

if WITH_KURL
libexec_PROGRAMS += libkurl.so
endif

if WITH_KMSN
libexec_PROGRAMS += libkmsn.so
endif

if HAVE_LIBNET
libexec_PROGRAMS += libredirect.so
endif


libredirect_so_SOURCES = http_redirect.c
libredirect_so_LDADD = -lnet

libkqq_so_SOURCES = kqq.c
libkurl_so_SOURCES = kurl.cpp
libkmsn_so_SOURCES = kmsn.cpp

#libexec_PROGRAMS +=   klz.so kjincheng.so
#AM_CPPFLAGS += -DENABLE_HOTEL 


#klz_so_SOURCES = klz.cpp
#klz_so_DEPENDENIES= libksql.so

#kjincheng_so_SOURCES = kServer.cpp kServer.hpp





