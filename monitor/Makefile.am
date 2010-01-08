AUTOMAKE_OPTIONS = foreign


sbindir=/etc/init.d/

bin_PROGRAMS = monitor 
bin_SCRIPTS = runmonitor
sbin_SCRIPTS = monitord

monitor_SOURCES = ip_filter.cpp main.cpp protocol_def.h 
						 
monitor_DEPENDENCIES = monitord

monitor_CXXFLAGS = -I $(top_srcdir)/libdreamtop

monitor_LDADD = -L$(top_builddir)/libdreamtop -ldreamtop -lpcap 
monitor_LDFLAGS= -Wl,-rpath,${prefix}/libexec/monitor/modules 
