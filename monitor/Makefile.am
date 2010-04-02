AUTOMAKE_OPTIONS = foreign


sbindir=/etc/init.d/

bin_PROGRAMS = monitor 
bin_SCRIPTS = runmonitor
sbin_SCRIPTS = monitord

EXTRA_DIST = runmonitor monitord

monitor_CXXFLAGS = @GLIB2_CFLAGS@ -I$(top_srcdir)/libdreamtop

monitor_SOURCES = ip_filter.cpp main.cpp auto_update.cpp protocol_def.h 
						 
if ENABLE_HOTEL
monitor_CXXFLAGS += -DENABLE_HOTEL
endif
monitor_LDADD = -L$(top_builddir)/libdreamtop -ldreamtop -lpcap
monitor_LDFLAGS= -Wl,-rpath,${prefix}/libexec/monitor/modules 
