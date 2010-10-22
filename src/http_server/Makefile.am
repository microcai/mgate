AUTOMAKE_OPTIONS = foreign

AM_CPPFLAGS = @NEED_ALL_CFLAGS@ -I$(srcdir)/../sqlclient -I$(srcdir)/.. -I$(srcdir)/../sms

if BUILD_HTTP_SERVER

noinst_LIBRARIES = libhttp_server.a

libhttp_server_a_SOURCES = http_server.c http_server.h
libhttp_server_a_SOURCES += htmlnode.c htmlnode.h
libhttp_server_a_SOURCES += html_paths.h
libhttp_server_a_SOURCES += html_path_info.c
libhttp_server_a_SOURCES += html_path_login.c
libhttp_server_a_SOURCES += html_path_favicon.c

libhttp_server_a_LIBADD =  favicon.o

libhttp_server_a_DEPENDENCIES = favicon.o

EXTRA_libhttp_server_a_SOURCES = favicon.ico

endif

 
favicon.o: favicon.ico
	$(OBJCOPY)	-I binary -B @obj_machine@ -O @obj_format@ $< $@
