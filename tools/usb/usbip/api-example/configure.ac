dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT([usbws-utils], [0.1.0], [linux-usb@vger.kernel.org])

AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE([foreign])
LT_INIT

# Silent build for automake >= 1.11
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_SUBST([EXTRA_CFLAGS], ["-Wall -Werror -Wextra -std=gnu99"])

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_MAKE_SET

# Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_CHECK_HEADERS([fcntl.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T

# Checks for library functions.
AC_CHECK_FUNCS([socket])

# Using libusb
AC_MSG_CHECKING([whether to use libusb])
AC_ARG_WITH([libusb],
	    [  --with-libusb           with libusb],
	    [with_libusb=yes],
	    [with_libusb=no])
AC_MSG_RESULT([$with_libusb])
AM_CONDITIONAL([WITH_LIBUSB], [test x$with_libusb = xyes])

AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT
