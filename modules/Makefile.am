#AUTOMAKE_OPTIONS = foreign

libexecdir=${prefix}/libexec/monitor/modules


MODULE_COMM_FLAGS = -I$(top_srcdir)/libdreamtop -fpic
MODULE_COMM_LD = -lpthread -L$(top_builddir)/libdreamtop -ldreamtop

libexec_PROGRAMS = klz.so  kmsn.so kpolice.so  kqq.so  kurl.so  

klz_so_SOURCES = klz.cpp
klz_so_CXXFLAGS = $(MODULE_COMM_FLAGS)
klz_so_LDADD = $(MODULE_COMM_LD)
klz_so_DEPENDENIES= libksql.so

kmsn_so_SOURCES = kmsn.cpp
kmsn_so_CXXFLAGS = $(MODULE_COMM_FLAGS)
kmsn_so_LDADD = $(MODULE_COMM_LD)
kmsn_so_DEPENDENIES= libksql.so

kpolice_so_SOURCES = kpolice.cpp kpolice.h
kpolice_so_CXXFLAGS = $(MODULE_COMM_FLAGS)
kpolice_so_LDADD = $(MODULE_COMM_LD)
kpolice_so_DEPENDENIES= libksql.so

kqq_so_SOURCES = kqq.cpp
kqq_so_CXXFLAGS = $(MODULE_COMM_FLAGS)
kqq_so_LDADD = $(MODULE_COMM_LD)
kqq_so_DEPENDENIES= libksql.so

kurl_so_SOURCES = kurl.cpp
kurl_so_CXXFLAGS = $(MODULE_COMM_FLAGS)
kurl_so_LDADD = $(MODULE_COMM_LD)
kurl_so_DEPENDENIES= libksql.so

kmsn_so_LDFLAGS= -shared -g 
klz_so_LDFLAGS=-shared -g
kurl_so_LDFLAGS=-shared -g 
kqq_so_LDFLAGS=-shared -g
kpolice_so_LDFLAGS=-shared -g

