#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.63])
AC_INIT([monitor], [1.9.0], [BUG-REPORT-ADDRESS])

AC_CONFIG_MACRO_DIR([m4])

AC_ARG_ENABLE(debug,
                [AC_HELP_STRING([--enable-debug],
                	[Enable DEBUG output. This feature which should be only enabled by developers])
                ],
               [
               CXXFLAGS+=" -g2 -DDEBUG -Wall -O0"
               CFLAGS+=" -g2 -DDEBUG -Wall -O0"
                 ]
              )
# Checks for programs.
AC_GNU_SOURCE
AC_PROG_CC_C99
AC_PROG_CXX
AC_CANONICAL_TARGET
AC_CANONICAL_SYSTEM
AM_INIT_AUTOMAKE([-Wall -Werror])

# define GETTEXT_* variables
GETTEXT_PACKAGE="$PACKAGE_NAME"
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [Define to the read-only architecture-independent data directory.])

AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION(0.16.1)

GTK_DOC_CHECK([1.11],[--flavour no-tmpl])


AC_ARG_WITH(mysql,[AC_HELP_STRING([--without-mysql],[disable support for mysql])],
		[ with_mysql=$withval ],[with_mysql=check])

AC_ARG_WITH(mysql-libs,[AC_HELP_STRING([--with-mysql-libs],[supply non-default LD flags for mysql])],
		[ MYSQL_LIBS=$withval ])

AC_ARG_WITH(mysql-cflags,[AC_HELP_STRING([--with-mysql-cflags],[supply non-default CPP flags for mysql])],
		[ MYSQL_CFLAGS=$withval ])
		
AC_ARG_WITH(jincheng,[AC_HELP_STRING([--without-jincheng],[disable jincheng module])],[],[with_jincheng="yes"])

AC_ARG_WITH(kqq, [AC_HELP_STRING([--with-kqq], [enable kqq module (default)])],[with_kqq=$withval],[with_kqq="yes"])
AC_ARG_WITH(kurl,[AC_HELP_STRING([--with-kurl],[enable kkurl module (default)])],[with_kurl=$withval],[with_kurl="yes"])
AC_ARG_WITH(kmsn,[AC_HELP_STRING([--with-kmsn],[enable kmsn module (default)])],[with_kmsn-$withval],[with_kmsn="yes"])


AM_CONDITIONAL([WITH_KQQ],[test x$with_kqq=xyes])
AM_CONDITIONAL([WITH_KURL],[test x$with_kurl=xyes])
AM_CONDITIONAL([WITH_KMSN],[test x$with_kmsn=xyes])


AC_SUBST(MODULES_PATH,/usr/libexec/)

AC_CONFIG_HEADERS([config.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T

# Checks for library functions.
PKG_CHECK_MODULES(LIBSOUP, [libsoup-2.4 >= 2.26])
PKG_CHECK_MODULES(GLIB2, [glib-2.0 >= 2.18 gthread-2.0 gobject-2.0 gmodule-2.0 gio-2.0 ])
PKG_CHECK_MODULES(NEED_ALL, [libsoup-2.4 >= 2.26 glib-2.0 >= 2.18 gthread-2.0 gobject-2.0 gmodule-2.0 gio-2.0 ])
AC_CHECK_LIB([pthread], [main])
AC_CHECK_LIB([pcap], [main],[],[AC_MSG_ERROR(need libpcap)])
AC_CHECK_LIB([pcap], [main],[AC_CHECK_HEADER(pcap.h)],[AC_MSG_ERROR(need libpcap)])


AC_ARG_ENABLE(hotel,[AC_HELP_STRING([--enable-hotel],[enabel build of hotel specific feature, need libnet])],[enable_hotel=$withval],[enable_hotel=no])

AS_IF( [ test "x$enable_hotel" != xno ],
	[
		AC_CHECK_LIB([net], [main],[ AC_CHECK_HEADER( libnet.h) ],[AC_MSG_ERROR(need libnet)])
		
		# Checks for library functions.
		AC_CHECK_HEADER([libnet/libnet.h],[],[])
		AC_CHECK_LIB(net,libnet_init,[have_libnet=yes],[])		
	],
	[
		have_libnet=no
	])

AS_IF([test "x${have_libnet}" = "xyes" ],
	[
		AM_CONDITIONAL(HAVE_LIBNET,[true])
	],
	[
		AM_CONDITIONAL(HAVE_LIBNET,[false])
	]
)

AS_IF(	[ test -n "${MYSQL_LIBS}" || test -n "${MYSQL_CFLAGS}"] ,[
		AM_CONDITIONAL([WITH_MYSQL],[true])
		AC_DEFINE(HAVE_MYSQL_H,[1],[Define to 1 if you have the <mysql/mysql.h> header file.])
		AC_CHECK_LIB(m,[main])
	] , [

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

])

AC_SUBST(MYSQL_CFLAGS)
AC_SUBST(MYSQL_LIBS)

AC_CHECK_TARGET_TOOL(OBJCOPY,objcopy,[],[])

case "$target" in
i386-* | i686-* )
AC_SUBST(obj_format,elf32-i386)
AC_SUBST(obj_machine,i386)
;;
x86_64-*)
AC_SUBST(obj_format,elf64-x86-64) 
AC_SUBST(obj_machine,i386:x86-64)
;;
esac


AC_CHECK_FUNCS([inet_ntoa memset munmap setenv socket strerror strstr])
# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h memory.h netinet/in.h stdlib.h string.h sys/ioctl.h sys/socket.h unistd.h])

AC_CHECK_HEADERS([net/ethernet.h netinet/ip.h netinet/tcp.h netinet/udp.h ],[],[AC_MSG_FAILURE(missing header files, see config.log)])

AC_CHECK_DECL(_SC_NPROCESSORS_ONLN,[],[AC_MSG_FAILURE(_SC_NPROCESSORS_ONLN not found,1)])

#AC_CONFIG_FILES([modules/Makefile])
AC_CONFIG_FILES( [					
					monitor.spec
					src/Makefile
					src/monitord
					modules/Makefile
					po/Makefile.in
					docs/Makefile					
					Makefile
		])

AS_IF([test "$prefix" = "NONE"],[prefix="/usr/local"])

AC_DEFINE_UNQUOTED(MODULES_PATH,"$prefix/lib/monitor/modules/",[the path of the loadable module])
AC_DEFINE_UNQUOTED(EXCUTE_PATH,"$prefix/bin/",[the path of the excutable])
AC_DEFINE_UNQUOTED(GETTEXT_LOCALE_PATH,"$prefix/share/locale",[the path of the locale])

AC_OUTPUT
