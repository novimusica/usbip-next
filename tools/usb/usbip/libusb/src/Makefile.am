AM_CPPFLAGS = -I$(top_srcdir)/../libsrc -DUSBIDS_FILE='"@USBIDS_DIR@/usb.ids"'
AM_CFLAGS   = -DDEBUG -DUSBIP_WITH_LIBUSB @EXTRA_CFLAGS@ \
	      -I$(top_srcdir)/../libsrc -I$(top_srcdir)/../src \
	      -I$(top_srcdir)/libsrc -I$(top_srcdir)/os \
	      -I$(top_srcdir)/stub
LDADD       = $(top_builddir)/libsrc/libusbip_libusb.la \
	      $(top_builddir)/stub/libusbip_stub.la

sbin_PROGRAMS := usbip_libusb usbipd_libusb
lib_LTLIBRARIES := libusbipc_libusb.la libusbipd_libusb.la

usbip_libusb_SOURCES := \
		../../src/usbip.h \
		../../src/usbip.c \
		../../src/usbip_network.c \
		../../src/usbip_connect.c \
		../../src/usbip_disconnect.c \
		../../src/usbip_list.c \
		../../src/usbip_bind.c \
		../../src/usbip_unbind.c
usbip_libusb_CFLAGS := $(AM_CFLAGS)

usbipd_libusb_SOURCES := \
		../../src/usbip_network.h \
		../../src/usbipd.c \
		../../src/usbipd_dev.c \
		../../src/usbip_network.c
usbipd_libusb_CFLAGS := $(AM_CFLAGS)

libusbipc_libusb_la_SOURCES := \
		../../src/usbip_network.c \
		../../src/usbip_connect.c \
		../../src/usbip_disconnect.c \
		../../src/usbip_list.c
libusbipc_libusb_la_CFLAGS := $(AM_CFLAGS) -DUSBIP_AS_LIBRARY

libusbipd_libusb_la_SOURCES := \
		../../src/usbipd_dev.c \
		../../src/usbip_network.c
libusbipd_libusb_la_CFLAGS := $(AM_CFLAGS) -DUSBIP_AS_LIBRARY
