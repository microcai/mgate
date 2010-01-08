#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.63])
AC_INIT([monitor], [1.0.0], [BUG-REPORT-ADDRESS])
AM_INIT_AUTOMAKE([-Wall -Werror])

#AC_CONFIG_SRCDIR([l])

AC_SUBST(MODULES_PATH,/usr/libexec/)

AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
# Checks for libraries.
# FIXME: Replace `main' with a function in `-lmicrocai':
AC_CHECK_LIB([mysqlclient], [main])
# FIXME: Replace `main' with a function in `-lpthread':
AC_CHECK_LIB([pthread], [main])
AC_CHECK_LIB([net], [main],[ AC_CHECK_HEADER( pcap.h) ],[	AC_ERROR(need libnet)])
AC_CHECK_LIB([pcap], [main],[AC_CHECK_HEADER(libnet.h)],[AC_ERROR(need libpcap)])

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h memory.h netinet/in.h stdlib.h string.h sys/ioctl.h sys/socket.h unistd.h])

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

#AC_CONFIG_FILES([modules/Makefile])
AC_CONFIG_FILES( [Makefile libmicrocai/Makefile monitor/Makefile modules/Makefile nat_helper/Makefile])
AC_OUTPUT
