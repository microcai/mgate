#AUTOMAKE_OPTIONS = foreign

libexecdir=${prefix}/libexec/monitor/modules

AM_CPPFLAGS = @GLIB2_CFLAGS@ -I$(top_srcdir)/src -fpic

LIBS = -lpthread -shared $(LDFLAGS)

libexec_PROGRAMS =  libredirect.so 

#kurl.so kqq.so kmsn.so

libredirect_so_SOURCES = http_redirect.c

libredirect_so_LDADD = -lnet


#libexec_PROGRAMS +=   klz.so kjincheng.so
#AM_CPPFLAGS += -DENABLE_HOTEL 


#klz_so_SOURCES = klz.cpp
#klz_so_DEPENDENIES= libksql.so

#kjincheng_so_SOURCES = kServer.cpp kServer.hpp

#kmsn_so_SOURCES = kmsn.cpp

#kqq_so_SOURCES = kqq.cpp

#kurl_so_SOURCES = kurl.cpp


