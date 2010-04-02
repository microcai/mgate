#AUTOMAKE_OPTIONS = foreign

libexecdir=${prefix}/libexec/monitor/modules


MODULE_COMM_FLAGS = @GLIB2_CFLAGS@ -I$(top_srcdir)/libdreamtop -fpic
MODULE_COMM_LD = -lpthread -L$(top_builddir)/libdreamtop -ldreamtop

libexec_PROGRAMS =  kpolice.so  kurl.so kqq.so kmsn.so
if ENABLE_HOTEL
libexec_PROGRAMS +=   klz.so kjincheng.so
MODULE_COMM_FLAGS += -DENABLE_HOTEL 

endif

if ENABLE_HOTEL

klz_so_SOURCES = klz.cpp
klz_so_CXXFLAGS = $(MODULE_COMM_FLAGS)
klz_so_LDADD = $(MODULE_COMM_LD)
klz_so_DEPENDENIES= libksql.so

kjincheng_so_SOURCES = kServer.cpp kServer.hpp
kjincheng_so_CXXFLAGS = $(MODULE_COMM_FLAGS)
kjincheng_so_LDADD = $(MODULE_COMM_LD)

endif

kmsn_so_SOURCES = kmsn.cpp
kmsn_so_CXXFLAGS = $(MODULE_COMM_FLAGS)
kmsn_so_LDADD = $(MODULE_COMM_LD)

kqq_so_SOURCES = kqq.cpp
kqq_so_CXXFLAGS = $(MODULE_COMM_FLAGS)
kqq_so_LDADD = $(MODULE_COMM_LD)

kpolice_so_SOURCES = kpolice.cpp kpolice.h
kpolice_so_CXXFLAGS = $(MODULE_COMM_FLAGS)
kpolice_so_LDADD = $(MODULE_COMM_LD)

kurl_so_SOURCES = kurl.cpp
kurl_so_CXXFLAGS = $(MODULE_COMM_FLAGS)
kurl_so_LDADD = $(MODULE_COMM_LD)

if ENABLE_HOTEL
klz_so_LDFLAGS=-shared $(LDFLAGS)
kjincheng_so_LDFLAGS= -shared $(LDFLAGS)
endif

kqq_so_LDFLAGS=-shared $(LDFLAGS)
kmsn_so_LDFLAGS= -shared $(LDFLAGS)
kurl_so_LDFLAGS=-shared $(LDFLAGS)
kpolice_so_LDFLAGS=-shared $(LDFLAGS)

