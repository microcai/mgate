#AUTOMAKE_OPTIONS = foreign

libexecdir=${libdir}/@PACKAGE_NAME@/modules

AM_CPPFLAGS = @GLIB2_CFLAGS@ -I$(top_srcdir)/src -fpic

LIBS = -lpthread -shared $(LDFLAGS)


libexec_PROGRAMS =  kqq.so
#kurl.so  kmsn.so

if HAVE_LIBNET
libexec_PROGRAMS += libredirect.so 

libredirect_so_SOURCES = http_redirect.c

libredirect_so_LDADD = -lnet

endif

#libexec_PROGRAMS +=   klz.so kjincheng.so
#AM_CPPFLAGS += -DENABLE_HOTEL 


#klz_so_SOURCES = klz.cpp
#klz_so_DEPENDENIES= libksql.so

#kjincheng_so_SOURCES = kServer.cpp kServer.hpp

#kmsn_so_SOURCES = kmsn.cpp

kqq_so_SOURCES = kqq.c

#kurl_so_SOURCES = kurl.cpp


