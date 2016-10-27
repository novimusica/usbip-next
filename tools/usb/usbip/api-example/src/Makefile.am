AM_CFLAGS =
AM_LDFLAGS = -L@libdir@

if !WITH_LIBUSB
LDFLAGS_DAEMON_DEV = -lusbip -lusbipd
LDFLAGS_DAEMON_APP = -lusbip -lusbipa
LDFLAGS_CMD = -lusbip -lusbipc

sbin_PROGRAMS = usbip_ex_daemon_dev usbip_ex_daemon_app \
		usbip_ex_cmd_connect usbip_ex_cmd_disconnect \
		usbip_ex_cmd_bind usbip_ex_cmd_unbind \
		usbip_ex_cmd_list_local \
		usbip_ex_cmd_attach usbip_ex_cmd_detach \
		usbip_ex_cmd_list_remote usbip_ex_cmd_port
else
LDFLAGS_DAEMON_DEV = -lusbip_libusb -lusbip_stub -lusbipd_libusb
LDFLAGS_DAEMON_APP =
LDFLAGS_CMD = -lusbip_libusb -lusbip_stub -lusbipc_libusb

sbin_PROGRAMS = usbip_ex_daemon_dev \
		usbip_ex_cmd_connect usbip_ex_cmd_disconnect \
		usbip_ex_cmd_bind usbip_ex_cmd_unbind \
		usbip_ex_cmd_list_local
endif

usbip_ex_daemon_dev_SOURCES = usbip_ex_daemon.c \
		usbip_ex_net_common.c usbip_ex_net_server.c
usbip_ex_daemon_dev_LDFLAGS = $(LDFLAGS_DAEMON_DEV)
usbip_ex_daemon_dev_CFLAGS = $(AM_CFLAGS)

if !WITH_LIBUSB
usbip_ex_daemon_app_SOURCES = usbip_ex_daemon.c \
		usbip_ex_net_common.c usbip_ex_net_server.c
usbip_ex_daemon_app_LDFLAGS = $(LDFLAGS_DAEMON_APP)
usbip_ex_daemon_app_CFLAGS = $(AM_CFLAGS)
endif

usbip_ex_cmd_connect_SOURCES = usbip_ex_cmd_connect.c \
		usbip_ex_net_common.c usbip_ex_net_client.c
usbip_ex_cmd_connect_LDFLAGS = $(LDFLAGS_CMD)
usbip_ex_cmd_connect_CFLAGS = $(AM_CFLAGS)

usbip_ex_cmd_disconnect_SOURCES = usbip_ex_cmd_disconnect.c \
		usbip_ex_net_common.c usbip_ex_net_client.c
usbip_ex_cmd_disconnect_LDFLAGS = $(LDFLAGS_CMD)
usbip_ex_cmd_disconnect_CFLAGS = $(AM_CFLAGS)

usbip_ex_cmd_bind_SOURCES = usbip_ex_cmd_bind.c
usbip_ex_cmd_bind_LDFLAGS = $(LDFLAGS_CMD)
usbip_ex_cmd_bind_CFLAGS = $(AM_CFLAGS)

usbip_ex_cmd_unbind_SOURCES = usbip_ex_cmd_unbind.c
usbip_ex_cmd_unbind_LDFLAGS = $(LDFLAGS_CMD)
usbip_ex_cmd_unbind_CFLAGS = $(AM_CFLAGS)

usbip_ex_cmd_list_local_SOURCES = usbip_ex_cmd_list_local.c \
		usbip_ex_net_common.c usbip_ex_net_client.c
usbip_ex_cmd_list_local_LDFLAGS = $(LDFLAGS_CMD)
usbip_ex_cmd_list_local_CFLAGS = $(AM_CFLAGS)

if !WITH_LIBUSB
usbip_ex_cmd_attach_SOURCES = usbip_ex_cmd_attach.c \
		usbip_ex_net_common.c usbip_ex_net_client.c
usbip_ex_cmd_attach_LDFLAGS = $(LDFLAGS_CMD)
usbip_ex_cmd_attach_CFLAGS = $(AM_CFLAGS)

usbip_ex_cmd_detach_SOURCES = usbip_ex_cmd_detach.c
usbip_ex_cmd_detach_LDFLAGS = $(LDFLAGS_CMD)
usbip_ex_cmd_detach_CFLAGS = $(AM_CFLAGS)

usbip_ex_cmd_list_remote_SOURCES = usbip_ex_cmd_list_remote.c \
		usbip_ex_net_common.c usbip_ex_net_client.c
usbip_ex_cmd_list_remote_LDFLAGS = $(LDFLAGS_CMD)
usbip_ex_cmd_list_remote_CFLAGS = $(AM_CFLAGS)

usbip_ex_cmd_port_SOURCES = usbip_ex_cmd_port.c
usbip_ex_cmd_port_LDFLAGS = $(LDFLAGS_CMD)
usbip_ex_cmd_port_CFLAGS = $(AM_CFLAGS)
endif
