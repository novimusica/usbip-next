SUBDIRS := libsrc src
includedir = @includedir@/usbip
include_HEADERS := $(addprefix libsrc/, \
		     usbip_common.h vhci_driver.h usbip_host_driver.h)

dist_man_MANS := $(addprefix doc/, usbip.8 usbipd.8 usbipa.8 \
		   usbip_sock_init.3 usbip_conn_init.3 \
		   usbip_break_all_connections.3 usbip_break_connection.3 \
		   usbip_set_use_debug.3 usbip_set_use_stderr.3 \
		   usbip_set_use_syslog.3 usbip_set_debug_flags.3 \
		   usbip_attach_device.3 usbip_detach_port.3 \
		   usbip_bind_device.3 usbip_unbind_device.3 \
		   usbip_list_imported_devices.3 \
		   usbip_list_importable_devices.3 \
		   usbip_list_local_devices.3 \
		   usbip_connect_device.3 usbip_disconnect_device.3 \
		   usbip_recv_pdu.3 usbip_driver_open.3 usbip_driver_close.3 \
		   usbip_update_driver.3)
