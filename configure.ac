#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.1])
AC_INIT([monitor], [1.1.5], [BUG-REPORT-ADDRESS])
AM_INIT_AUTOMAKE([-Wall -Werror])

AC_SUBST_FILE(RPATH_EXT)

AC_ARG_ENABLE(debug,
                [  --enable-debug Enable DEBUG output. This is a debugging feature
        which should not be only enabled by develpoers
                ],
                [ CXXFLAGS="$CFLAGS -g2 -O0 -DDEBUG -Wall"],
                [ CXXFLAGS="$CFLAGS -Os"]
              )
AC_ARG_ENABLE(hotel,
				[  --enable-hotel enable hotel related features and 
	 disable cafe related features],
				[AM_CONDITIONAL(ENABLE_HOTEL,true)],
				[AM_CONDITIONAL(ENABLE_HOTEL,false)])

AC_SUBST(MODULES_PATH,/usr/libexec/)

AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CXX
# Checks for library functions.
AC_CHECK_LIB([pthread], [main])

AC_CHECK_LIB([net], [main],[ AC_CHECK_HEADER( libnet.h) ],[AC_MSG_ERROR(need libnet)])
AC_CHECK_LIB([pcap], [main],[AC_CHECK_HEADER(pcap.h)],[AC_MSG_ERROR(need libpcap)])

AC_CHECK_LIB([mysqlclient -L/usr/lib/mysql], [mysql_query] ,
				#found
				[
					LD_MYSQL_FLAGS="-lmysqlclient -L/usr/lib/mysql";
				],
				#not found 
				[
					AC_CHECK_LIB([mysqlclient -L/usr/lib64/mysql], [mysql_query] ,
									#found
									[
										LD_MYSQL_FLAGS="-lmysqlclient -L/usr/lib64/mysql"
									],
									#not found
									[
										AC_CHECK_LIB([mysqlclient -L/usr/local/lib/mysql], [mysql_query],
														#found
														[LD_MYSQL_FLAGS="-lmysqlclient -L/usr/local/lib/mysql"],
														#not found
														[
															AC_CHECK_LIB([mysqlclient -L/usr/local/lib64/mysql], [mysql_query],
																			#found
																			[LD_MYSQL_FLAGS="-lmysqlclient -L/usr/local/lib64/mysql" ],
																			#not found
																			[
																				AC_MSG_ERROR(monitor needs mysqlclient developing package,1)
																			])														
														])
									])
				])

AC_SUBST(LD_MYSQL_FLAGS)

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_MMAP
AC_CHECK_FUNCS([inet_ntoa memset munmap setenv socket strerror strstr])
# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h memory.h netinet/in.h stdlib.h string.h sys/ioctl.h sys/socket.h unistd.h])

#AC_CONFIG_FILES([modules/Makefile])
AC_CONFIG_FILES( [Makefile libdreamtop/Makefile monitor/Makefile modules/Makefile])

if  [ $prefix = "NONE" ]; then
AC_DEFINE_UNQUOTED(MODULES_PATH,"/usr/local/libexec/monitor/modules/",the path of the loadable module)
AC_DEFINE_UNQUOTED(EXCUTE_PATH,"/usr/local/bin/",the path of the excutable )

else

AC_DEFINE_UNQUOTED(MODULES_PATH,"$prefix/libexec/monitor/modules/",the path of the loadable module)
AC_DEFINE_UNQUOTED(EXCUTE_PATH,"$prefix/bin/",the path of the excutable )
fi

AC_OUTPUT
