#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.63])
AC_INIT([monitor], [1.2.0], [BUG-REPORT-ADDRESS])

AC_CONFIG_MACRO_DIR([m4])

AC_ARG_ENABLE(debug,
                [AC_HELP_STRING([--enable-debug],
                	[Enable DEBUG output. This feature which should be only enabled by developers])
                ],
                [ CXXFLAGS="-g2 -DDEBUG -Wall -O0"]
              )
# Checks for programs.
AC_PROG_CC_C99
AC_CANONICAL_TARGET
AM_INIT_AUTOMAKE([-Wall -Werror])

# define GETTEXT_* variables
GETTEXT_PACKAGE="$PACKAGE_NAME"
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [Define to the read-only architecture-independent data directory.])

AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION(0.16.1)

AC_ARG_WITH(mysql,[AC_HELP_STRING([--without-mysql],[disable support for mysql])],
		[ with_mysql=$withval ],[with_mysql=check])
		
AC_ARG_WITH(jincheng,[AC_HELP_STRING([--without-jincheng],[disable jincheng module])],[],[with_jincheng="yes"])
AC_ARG_WITH(kpolice ,[AC_HELP_STRING([--without-kpolice],[disable kpolice module])],[],[with_kpolice="yes"])

AC_ARG_WITH(kqq, [AC_HELP_STRING([--with-kqq], [enable kqq module (default)])],[],[with_kqq="yes"])
AC_ARG_WITH(kurl,[AC_HELP_STRING([--with-kurl],[enable kkurl module (default)])],[],[with_kurl="yes"])
AC_ARG_WITH(kmsn,[AC_HELP_STRING([--with-kmsn],[enable kmsn module (default)])],[],[with_kmsn="yes"])

AC_SUBST(MODULES_PATH,/usr/libexec/)

AC_CONFIG_HEADERS([config.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T

# Checks for library functions.

PKG_CHECK_MODULES(GLIB2, [glib-2.0 gthread-2.0 gobject-2.0 gmodule-2.0 gio-2.0 ])
AC_CHECK_LIB([pthread], [main])

AC_CHECK_LIB([pcap], [main],[AC_CHECK_HEADER(pcap.h)],[AC_MSG_ERROR(need libpcap)])

AS_IF( [ test "x$enable_hotel" != xno ],[AC_CHECK_LIB([net], [main],[ AC_CHECK_HEADER( libnet.h) ],[AC_MSG_ERROR(need libnet)])])

AS_IF([test "x$with_mysql" != "xno" ],
	[
		AC_CHECK_PROG(MYSQL_CONFIG,mysql_config,mysql_config)
		AS_IF([test "x${MYSQL_CONFIG}" = "xmysql_config"],[
			MYSQL_LIBS=`mysql_config --libs` ;
			MYSQL_CFLAGS=`mysql_config --cflags` ;
			AM_CONDITIONAL([WITH_MYSQL],[true])
		],[
			AM_CONDITIONAL([WITH_MYSQL],[false])
		])
	],[
		AM_CONDITIONAL([WITH_MYSQL],[false])
	])	

AC_CHECK_HEADERS([mysql/mysql.h])
AC_CHECK_HEADERS([mysql/mysql.h],[],[AM_CONDITIONAL([WITH_MYSQL],[false])])

AC_SUBST(MYSQL_CFLAGS)
AC_SUBST(MYSQL_LIBS)

# Checks for library functions.
AC_FUNC_MMAP
AC_CHECK_FUNCS([inet_ntoa memset munmap setenv socket strerror strstr])
# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h memory.h netinet/in.h stdlib.h string.h sys/ioctl.h sys/socket.h unistd.h])


#AC_CONFIG_FILES([modules/Makefile])
AC_CONFIG_FILES( [
					Makefile
					src/Makefile
					modules/Makefile
					po/Makefile.in
					monitor-hotel.spec
					monitor-cafe.spec
		])

AS_IF([test "$prefix" = "NONE"],[prefix="/usr/local"])

AC_DEFINE_UNQUOTED(MODULES_PATH,"$prefix/libexec/monitor/modules/",[the path of the loadable module])
AC_DEFINE_UNQUOTED(EXCUTE_PATH,"$prefix/bin/",[the path of the excutable])

AC_OUTPUT
