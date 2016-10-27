dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT([usbip-utils-libusb], [0.1.0], [linux-usb@vger.kernel.org])
AC_DEFINE([USBIP_VERSION], [0x00000111], [binary-coded decimal version number])

CURRENT=0
REVISION=1
AGE=0
AC_SUBST([LIBUSBIP_VERSION], [$CURRENT:$REVISION:$AGE], [library version])

AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE([foreign subdir-objects])
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
AC_CHECK_HEADERS([arpa/inet.h fcntl.h libudev.h netdb.h netinet/in.h dnl
		  pthread.h poll.h stdint.h stdlib.h dnl
		  string.h sys/socket.h syslog.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_INT32_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T

# Checks for declarations.
AC_CHECK_DECLS([container_of])
AC_CHECK_DECLS([P_tmpdir])

# Checks for library functions.
AC_FUNC_REALLOC
AC_CHECK_FUNCS([daemon fork memset mkdir regcomp socket strchr dnl
		strerror strstr strtoul syslog])

# Checks libusb
AC_CHECK_HEADER([libusb-1.0/libusb.h],
		[AC_CHECK_LIB([usb-1.0], [main],
			      [LIBS="$LIBS -lusb-1.0"],
        		      [AC_MSG_ERROR([libusb-1.0 not found!])])],
		[AC_MSG_ERROR([Missing libusb-1.0/libusb.h])])

# Checks usage of empty struct
AC_COMPILE_IFELSE([AC_LANG_SOURCE([
			#include <stdio.h>
			struct s {} v;
			int main(void) { return printf("%p", &v); }])],
		  [AC_MSG_RESULT([passed empty struct test])],
		  [AC_MSG_RESULT([failed empty struct test])
		   AC_DEFINE([USBIP_OS_NO_EMPTY_STRUCT],
			     [1], [usage of empty struct])])

# Checks container_of
AC_COMPILE_IFELSE([AC_LANG_SOURCE([
			#include <stddef.h>
			struct s { int a, b; } v;
			int main(void) {
			  struct s *p = container_of(&v.a, struct s, a);
			  return printf("%p", p);
			}])],
		  [AC_MSG_RESULT([have container_of])
		   AC_DEFINE([USBIP_OS_HAVE_CONTAINER_OF],
		 	     [1], [usage of container_of])],
		  [AC_MSG_RESULT([no container_of])])

# Checks for libwrap library.
AC_MSG_CHECKING([whether to use the libwrap (TCP wrappers) library])
AC_ARG_WITH([tcp-wrappers],
	    [AS_HELP_STRING([--with-tcp-wrappers],
			    [use the libwrap (TCP wrappers) library])],
	    dnl [ACTION-IF-GIVEN]
	    [if test "$withval" = "yes"; then
		     AC_MSG_RESULT([yes])
		     AC_MSG_CHECKING([for hosts_access in -lwrap])
		     saved_LIBS="$LIBS"
		     LIBS="-lwrap $saved_LIBS"
		     AC_TRY_LINK(
		       [int hosts_access(); int allow_severity, deny_severity;],
		       [hosts_access()],
		       [AC_MSG_RESULT([yes]);
			AC_DEFINE([HAVE_LIBWRAP], [1],
				  [use tcp wrapper]) wrap_LIB="-lwrap"],
		       [AC_MSG_RESULT([not found]); exit 1])
	     else
		     AC_MSG_RESULT([no]);
	     fi],
	    dnl [ACTION-IF-NOT-GIVEN]
	    [AC_MSG_RESULT([(default)])
	     AC_MSG_CHECKING([for hosts_access in -lwrap])
	     saved_LIBS="$LIBS"
	     LIBS="-lwrap $saved_LIBS"
	     AC_TRY_LINK(
	       [int hosts_access(); int allow_severity, deny_severity;],
	       [hosts_access()],
	       [AC_MSG_RESULT([yes]);
		AC_DEFINE([HAVE_LIBWRAP], [1], [use tcp wrapper])],
	       [AC_MSG_RESULT([no]); LIBS="$saved_LIBS"])])

# Sets directory containing usb.ids.
AC_ARG_WITH([usbids-dir],
	    [AS_HELP_STRING([--with-usbids-dir=DIR],
	       [where usb.ids is found (default /usr/share/hwdata/)])],
	    [USBIDS_DIR=$withval], [USBIDS_DIR="/usr/share/hwdata/"])
AC_SUBST([USBIDS_DIR])

# use _FORTIFY_SOURCE
AC_MSG_CHECKING([whether to use fortify])
AC_ARG_WITH([fortify],
	    [AS_HELP_STRING([--with-fortify],
			    [use _FORTIFY_SROUCE option when compiling)])],
			    dnl [ACTION-IF-GIVEN]
			    [if test "$withval" = "yes"; then
				AC_MSG_RESULT([yes])
				CFLAGS="$CFLAGS -D_FORTIFY_SOURCE -O"
			     else
			     	AC_MSG_RESULT([no])
				CFLAGS="$CFLAGS -U_FORTIFY_SOURCE"
			     fi
			    ],
			    dnl [ACTION-IF-NOT-GIVEN]
			    [AC_MSG_RESULT([default])])

# use dummy device driver
AC_MSG_CHECKING([whether to use dummy driver])
AC_ARG_WITH([dummy-driver],
	    [AS_HELP_STRING([--with-dummy-driver],
			    [use dummy device driver instead of vUDC ])],
			    dnl [ACTION-IF-GIVEN]
			    [if test "$withval" = "yes"; then
				dummy_driver=yes
			     else
				dummy_driver=no
			     fi
			    ],
			    dnl [ACTION-IF-NOT-GIVEN]
			    dummy_driver=no)
AC_MSG_RESULT([$dummy_driver])
AM_CONDITIONAL([USE_DUMMY_DRIVER], [test x$dummy_driver = xyes])

AC_CONFIG_FILES([Makefile stub/Makefile
			  libsrc/Makefile src/Makefile])
AC_OUTPUT
