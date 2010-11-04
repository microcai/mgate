AUTOMAKE_OPTIONS = foreign

DIST_SUBDIRS = misc sqlclient http_server

SUBDIRS = misc sqlclient @http_server@

AM_CPPFLAGS = @GLIB2_CFLAGS@ -I$(srcdir)/sqlclient

STATIC_LIBS = 	$(builddir)/sqlclient/libsqlclient.a \
			@http_server_lib@ $(builddir)/misc/libmisc.a	
			

LIBS += @NEED_ALL_LIBS@ @MYSQL_LIBS@ @SQLITE3_LIBS@ 

DISTCLEANFILES = mgated

sbindir=/etc/init.d/

bin_PROGRAMS = mgate 
sbin_SCRIPTS = mgated

EXTRA_DIST =  mgated.in

mgate_SOURCES =main.c global.h i18n.h

mgate_SOURCES += pcap_thread.c  pcap_thread.h 
mgate_SOURCES += pcap_hander.c pcap_hander.h
mgate_SOURCES += clientmgr.c clientmgr.h
mgate_SOURCES += utils.c utils.h
mgate_SOURCES += module.c module.h
mgate_SOURCES += kpolice.c kpolice.h
mgate_SOURCES += socket/g_socket_client_proxy.c socket/g_socket_client_proxy.h
mgate_SOURCES += socket/g_socket_source.c socket/g_socket_source.h

mgate_SOURCES += ksql.h ksql.c
mgate_SOURCES += traffic_status.c traffic_status.h

mgate_LDADD =  ${STATIC_LIBS}

mgate_DEPENDENCIES = ${STATIC_LIBS}
