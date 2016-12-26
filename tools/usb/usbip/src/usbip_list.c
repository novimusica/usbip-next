/*
 * Copyright (C) 2011 matt mooney <mfm@muteddisk.com>
 *               2005-2007 Takahiro Hirofuchi
 * Copyright (C) 2015-2016 Samsung Electronics
 *               Igor Kotrasinski <i.kotrasinsk@samsung.com>
 *               Krzysztof Opasiak <k.opasiak@samsung.com>
 * Copyright (C) 2015-2016 Nobuo Iwata <nobuo.iwata@fujixerox.co.jp>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "usbip_config.h"

#include <sys/types.h>

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef USBIP_AS_LIBRARY
#include <getopt.h>
#endif

#ifndef USBIP_OS_NO_DIRENT_H
#include <dirent.h>
#endif

#include "usbip_host_driver.h"
#include "usbip_device_driver.h"
#include "usbip_common.h"
#include "usbip_network.h"
#include "usbip.h"

#ifndef USBIP_AS_LIBRARY
static const char usbip_list_usage_string[] =
	"usbip list [-p|--parsable] <args>\n"
	"    -p, --parsable         Parsable list format\n"
	"    -r, --remote=<host>    List the importable USB devices on <host>\n"
	"    -l, --local            List the local USB devices\n"
	"    -d, --device           List the local devices with "
	"an alternate driver,\n"
	"                           e.g. vUDC\n";

void usbip_list_usage(void)
{
	printf("usage: %s", usbip_list_usage_string);
}
#endif

#ifndef USBIP_WITH_LIBUSB
static int get_importable_devices(const char *host, struct usbip_sock *sock)
{
	char product_name[100];
	char class_name[100];
	struct op_devlist_reply reply;
	uint16_t code = OP_REP_DEVLIST;
	struct usbip_usb_device udev;
	struct usbip_usb_interface uintf;
	unsigned int i;
	int rc, j;

	rc = usbip_net_send_op_common(sock, OP_REQ_DEVLIST, 0);
	if (rc < 0) {
		dbg("usbip_net_send_op_common failed");
		return -1;
	}

	rc = usbip_net_recv_op_common(sock, &code);
	if (rc < 0) {
		dbg("usbip_net_recv_op_common failed");
		return -1;
	}

	memset(&reply, 0, sizeof(reply));
	rc = usbip_net_recv(sock, &reply, sizeof(reply));
	if (rc < 0) {
		dbg("usbip_net_recv_op_devlist failed");
		return -1;
	}
	PACK_OP_DEVLIST_REPLY(0, &reply);
	dbg("importable devices: %d\n", reply.ndev);

	if (reply.ndev == 0) {
		info("no importable devices found on %s", host);
		return 0;
	}

	printf("Importable USB devices\n");
	printf("======================\n");
	printf(" - %s\n", host);

	for (i = 0; i < reply.ndev; i++) {
		memset(&udev, 0, sizeof(udev));
		rc = usbip_net_recv(sock, &udev, sizeof(udev));
		if (rc < 0) {
			dbg("usbip_net_recv failed: usbip_usb_device[%d]", i);
			return -1;
		}
		usbip_net_pack_usb_device(0, &udev);

		usbip_names_get_product(product_name, sizeof(product_name),
					udev.idVendor, udev.idProduct);
		usbip_names_get_class(class_name, sizeof(class_name),
				      udev.bDeviceClass, udev.bDeviceSubClass,
				      udev.bDeviceProtocol);
		printf("%11s: %s\n", udev.busid, product_name);
		printf("%11s: %s\n", "", udev.path);
		printf("%11s: %s\n", "", class_name);

		for (j = 0; j < udev.bNumInterfaces; j++) {
			rc = usbip_net_recv(sock, &uintf, sizeof(uintf));
			if (rc < 0) {
				err("usbip_net_recv failed: usbip_usb_intf[%d]",
						j);

				return -1;
			}
			usbip_net_pack_usb_interface(0, &uintf);

			usbip_names_get_class(class_name, sizeof(class_name),
					uintf.bInterfaceClass,
					uintf.bInterfaceSubClass,
					uintf.bInterfaceProtocol);
			printf("%11s: %2d - %s\n", "", j, class_name);
		}

		printf("\n");
	}

	return 0;
}

int usbip_list_importable_devices(const char *host, const char *port)
{
	int rc;
	struct usbip_sock *sock;

	if (usbip_names_init(USBIDS_FILE))
		err("failed to open %s", USBIDS_FILE);

	sock = usbip_conn_open(host, port);
	if (!sock) {
		err("could not connect to %s:%s", host, port);
		goto err_out;
	}
	dbg("connected to %s:%s", host, port);

	rc = get_importable_devices(host, sock);
	if (rc < 0) {
		err("failed to get device list from %s", host);
		goto err_conn_close;
	}

	usbip_conn_close(sock);
	usbip_names_free();
	return 0;

err_conn_close:
	usbip_conn_close(sock);
err_out:
	usbip_names_free();
	return -1;
}
#else
int usbip_list_importable_devices(const char *host, const char *port)
{
	(void)port;

	info("unsupported command: list --remote %s", host);
	return 0;
}
#endif /* !USBIP_WITH_LIBUSB */

static void print_device(const char *busid, uint16_t vendor,
			 uint16_t product, int parsable)
{
	if (parsable)
		printf("busid=%s#usbid=%04x:%04x#", busid, vendor, product);
	else
		printf(" - busid %s (%04x:%04x)\n", busid, vendor, product);
}

static void print_product_name(const char *product_name, int parsable)
{
	if (!parsable)
		printf("   %s\n", product_name);
}

int usbip_list_local_devices(int parsable)
{
	struct usbip_usb_device *udevs;
	int num, i;
	char product_name[128];

	if (usbip_names_init(USBIDS_FILE))
		err("failed to open %s", USBIDS_FILE);

	if (usbip_driver_open()) {
		err("open driver");
		goto err_out;
	}

	num = usbip_list_devices(&udevs);
	if (num < 0)
		goto err_driver_close;

	for (i = 0; i < num; i++) {

		/* Get product name. */
		usbip_names_get_product(product_name, sizeof(product_name),
					(udevs + i)->idVendor,
					(udevs + i)->idProduct);

		/* Print information. */
		print_device((udevs + i)->busid, (udevs + i)->idVendor,
			     (udevs + i)->idProduct, parsable);
		print_product_name(product_name, parsable);

		printf("\n");

	}

	free(udevs);
	usbip_driver_close();
	usbip_names_free();
	return 0;

err_driver_close:
	usbip_driver_close();
err_out:
	usbip_names_free();
	return -1;
}

#ifndef USBIP_AS_LIBRARY
int usbip_list(int argc, char *argv[])
{
	static const struct option opts[] = {
		{ "parsable", no_argument,       NULL, 'p' },
		{ "remote",   required_argument, NULL, 'r' },
		{ "local",    no_argument,       NULL, 'l' },
		{ "device",    no_argument,       NULL, 'd' },
		{ NULL,       0,                 NULL,  0  }
	};

	int parsable = 0;
	int opt;
	int ret = -1;

	for (;;) {
		opt = getopt_long(argc, argv, "pr:ld", opts, NULL);

		if (opt == -1)
			break;

		switch (opt) {
		case 'p':
			parsable = 1;
			break;
		case 'r':
			ret = usbip_list_importable_devices(optarg,
							    usbip_port_string);
			goto out;
		case 'l':
			ret = usbip_list_local_devices(parsable);
			goto out;
		case 'd':
			usbip_hdriver_set(USBIP_HDRIVER_TYPE_DEVICE);
			ret = usbip_list_local_devices(parsable);
			goto out;
		default:
			goto err_out;
		}
	}

err_out:
	usbip_list_usage();
out:
	return ret;
}
#endif /* !USBIP_AS_LIBRARY */
