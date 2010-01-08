#
#  Copyright 2009 microcai <microcai@sina.com>
# 
#  See COPYING for more details about this software's license
# 


libexecdir=${prefix}/libexec/monitor/modules

libexec_PROGRAMS = libdreamtop.so

libdreamtop_so_CXXFLAGS = -fpic

libdreamtop_so_SOURCES = libmicrocai.cpp \
						 CString.cpp CString.h \
						 libdreamtop.h functions.h \
						 libmicrocai-macros.h libmicrocai-queue.h \
						 libmicrocai-types.h \
						 Parameters.cpp load_module.cpp kmysql.cpp kmysql.h \
						 protocol_handler.cpp 
						 
if ENABLE_HOTEL
libdreamtop_so_SOURCES += clients.cpp http_redirect.cpp	
libdreamtop_so_CXXFLAGS += -DENABLE_HOTEL
endif 
						
libdreamtop_so_LDADD = -ldl -lnet -lpthread $(LD_MYSQL_FLAGS)
libdreamtop_so_LDFLAGS = -shared

