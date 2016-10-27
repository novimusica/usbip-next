libusbip_stub_la_CPPFLAGS =
libusbip_stub_la_CFLAGS   = -DDEBUG -DUSBIP_WITH_LIBUSB @EXTRA_CFLAGS@ \
	-I$(top_srcdir)/../libsrc \
	-I$(top_srcdir)/libsrc -I$(top_srcdir)/os
libusbip_stub_la_LDFLAGS  = -version-info @LIBUSBIP_VERSION@ -lpthread -lusb-1.0

lib_LTLIBRARIES = libusbip_stub.la
libusbip_stub_la_SOURCES = stub_common.c stub_common.h \
	stub_main.c stub_dev.c stub_tx.c stub_rx.c stub_event.c stub.h
