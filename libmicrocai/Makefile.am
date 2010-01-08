#AUTOMAKE_OPTIONS = foreign

libdir=$(PREFIX)/monitor/modules

noinst_PROGRAMS = libmicrocai.so
		 
libmicrocai_so_SOURCES = clients.cpp libmicrocai.cpp \
						libmicrocai.h libmicrocai-functions.h \
						libmicrocai-macros.h libmicrocai-queue.h \
						libmicrocai-types.h my_log.cpp my_log.h \
						Parameters.cpp
						
libmicrocai_so_LDADD = -ldl
libmicrocai_so_LDFLAGS = -shared
libmicrocai_so_CXXFLAGS = -fpic
