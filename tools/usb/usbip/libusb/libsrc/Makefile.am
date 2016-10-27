libusbip_libusb_la_CPPFLAGS = -DUSBIDS_FILE='"@USBIDS_DIR@/usb.ids"'
libusbip_libusb_la_CFLAGS   = -DDEBUG -DUSBIP_WITH_LIBUSB @EXTRA_CFLAGS@ \
			      -I$(top_srcdir)/os -I$(top_srcdir)/../libsrc
libusbip_libusb_la_LDFLAGS  = -version-info @LIBUSBIP_VERSION@

if USE_DUMMY_DRIVER
DEV_DRV = ../../libsrc/usbip_device_driver.h \
	  ./dummy_device_driver.c
else
DEV_DRV = ../../libsrc/usbip_host_common.h \
	  ../../libsrc/usbip_host_common.c \
	  ../../libsrc/usbip_device_driver.h \
	  ../../libsrc/usbip_device_driver.c \
	  ../../libsrc/usbip_ux.h \
	  ../../libsrc/usbip_ux.c \
	  ../../libsrc/sysfs_utils.h \
	  ../../libsrc/sysfs_utils.c
endif

lib_LTLIBRARIES = libusbip_libusb.la
libusbip_libusb_la_SOURCES = \
	../../libsrc/names.h \
	../../libsrc/names.c \
	../../libsrc/usbip_common.h \
	../../libsrc/usbip_common.c \
	../../libsrc/usbip_host_api.c \
	$(DEV_DRV)
