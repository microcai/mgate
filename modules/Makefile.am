#AUTOMAKE_OPTIONS = foreign

libexecdir=${libdir}/@PACKAGE_NAME@/modules

AM_CPPFLAGS = @GLIB2_CFLAGS@ -I$(top_srcdir)/src -fpic

LIBS += @GLIB2_LIBS@ -shared


libexec_PROGRAMS =  libkqq.so libkurl.so
#  libkmsn.so

if HAVE_LIBNET
libexec_PROGRAMS += libredirect.so 

libredirect_so_SOURCES = http_redirect.c

libredirect_so_LDADD = -lnet

endif

libkqq_so_SOURCES = kqq.c

libkurl_so_SOURCES = kurl.cpp

#libexec_PROGRAMS +=   klz.so kjincheng.so
#AM_CPPFLAGS += -DENABLE_HOTEL 


#klz_so_SOURCES = klz.cpp
#klz_so_DEPENDENIES= libksql.so

#kjincheng_so_SOURCES = kServer.cpp kServer.hpp

#libkmsn_so_SOURCES = kmsn.cpp



