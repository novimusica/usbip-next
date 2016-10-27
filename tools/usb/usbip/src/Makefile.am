AM_CPPFLAGS = -I$(top_srcdir)/libsrc -DUSBIDS_FILE='"@USBIDS_DIR@/usb.ids"'
AM_CFLAGS   = @EXTRA_CFLAGS@
LDADD       = $(top_builddir)/libsrc/libusbip.la

sbin_PROGRAMS := usbip usbipd usbipa
lib_LTLIBRARIES := libusbipc.la libusbipd.la libusbipa.la

usbip_SOURCES := usbip.h usbip.c usbip_network.c \
		 usbip_attach.c usbip_detach.c usbip_list.c \
		 usbip_bind.c usbip_unbind.c usbip_port.c \
		 usbip_connect.c usbip_disconnect.c
usbip_CFLAGS := $(AM_CFLAGS)

usbipd_SOURCES := usbip_network.h usbipd.c usbipd_dev.c usbip_network.c
usbipd_CFLAGS := $(AM_CFLAGS)

usbipa_SOURCES := usbip_network.h usbipd.c usbipd_app.c usbip_network.c
usbipa_CFLAGS := $(AM_CFLAGS) -DUSBIP_DAEMON_APP

libusbipc_la_SOURCES := usbip_network.c \
			usbip_attach.c usbip_detach.c usbip_list.c \
			usbip_port.c \
			usbip_connect.c usbip_disconnect.c
libusbipc_la_CFLAGS := $(AM_CFLAGS) -DUSBIP_AS_LIBRARY
libusbipc_la_LDFLAGS := -version-info @LIBUSBIP_VERSION@

libusbipd_la_SOURCES := usbipd_dev.c usbip_network.c
libusbipd_la_CFLAGS := $(AM_CFLAGS) -DUSBIP_AS_LIBRARY
libusbipd_la_LDFLAGS := -version-info @LIBUSBIP_VERSION@

libusbipa_la_SOURCES := usbipd_app.c usbip_network.c
libusbipa_la_CFLAGS := $(AM_CFLAGS) -DUSBIP_AS_LIBRARY
libusbipa_la_LDFLAGS := -version-info @LIBUSBIP_VERSION@
