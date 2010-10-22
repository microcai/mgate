AUTOMAKE_OPTIONS = foreign

AM_CPPFLAGS = @GLIB2_CFLAGS@ @MYSQL_CFLAGS@ -I$(srcdir)/../dbclient -I$(srcdir)/..

noinst_LIBRARIES = libsqlclient.a

libsqlclient_a_SOURCES = gsqlconnect.c gsqlconnect.h
libsqlclient_a_SOURCES += gsqlresult.c gsqlresult.h
libsqlclient_a_SOURCES += sqlbackend.c
		 
if WITH_MYSQL
  libsqlclient_a_SOURCES += gsqlconnect_mysql.c	 gsqlconnect_mysql.h
  libsqlclient_a_SOURCES += mysql_static_template.h
  AM_CPPFLAGS += -D__HAVE_MYSQL
endif

if WITH_SQLITE3
  libsqlclient_a_SOURCES += gsqlconnect_sqlite3.c	 gsqlconnect_sqlite3.h
  libsqlclient_a_SOURCES += sqlite_static_template.h
  AM_CPPFLAGS += -DWITH_SQLITE3
endif

