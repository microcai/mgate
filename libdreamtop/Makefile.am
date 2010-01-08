#
#  Copyright 2009 microcai <microcai@sina.com>
# 
#  See COPYING for more details about this software's license
# 


libexecdir=${prefix}/libexec/monitor/modules

libexec_PROGRAMS = libdreamtop.so
		 
libdreamtop_so_SOURCES = clients.cpp libmicrocai.cpp \
						libmicrocai.h libmicrocai-functions.h \
						libmicrocai-macros.h libmicrocai-queue.h \
						libmicrocai-types.h my_log.cpp my_log.h \
						Parameters.cpp load_module.cpp kmysql.cpp kmysql.h
						
libdreamtop_so_LDADD = -ldl $(LD_MYSQL_FLAGS)
libdreamtop_so_LDFLAGS = -shared
libdreamtop_so_CXXFLAGS = -fpic
