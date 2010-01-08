AUTOMAKE_OPTIONS = foreign

bin_PROGRAMS = monitor
monitor_SOURCES = ip_filter.cpp main.cpp protocol_def.h 
						 
monitor_DEPENDENCIES = 

monitor_CXXFLAGS = -I $(top_srcdir)/libdreamtop


monitor_LDADD = -L$(top_builddir)/libdreamtop -ldreamtop -lpcap 
monitor_LDFLAGS= -Wl,-rpath,${prefix}/libexec/monitor/modules,-rpath,./,-rpath,./build/libdreamtop,-rpath,./build/modules 
