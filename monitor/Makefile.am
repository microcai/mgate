AUTOMAKE_OPTIONS = foreign

AM_CPPFLAGS = @GLIB2_CFLAGS@ -I$(top_srcdir)/libdreamtop

LIBS=

sbindir=/etc/init.d/

bin_PROGRAMS = monitor 
bin_SCRIPTS = runmonitor
sbin_SCRIPTS = monitord

EXTRA_DIST = runmonitor monitord

monitor_SOURCES = ip_filter.c main.cpp auto_update.cpp protocol_def.h 
						 
if ENABLE_HOTEL
monitor_CXXFLAGS  = -DENABLE_HOTEL
endif
monitor_LDADD = @GLIB2_LIBS@ -L$(top_builddir)/libdreamtop -ldreamtop -lpcap
monitor_LDFLAGS= -Wl,-rpath,${prefix}/libexec/monitor/modules 
