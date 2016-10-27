/*
 * Copyright (C) 2015 Nobuo Iwata
 *               2011 matt mooney <mfm@muteddisk.com>
 *               2005-2007 Takahiro Hirofuchi
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

#include <sys/stat.h>

#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#ifndef USBIP_AS_LIBRARY
#include <getopt.h>
#endif

#include "usbip_host_driver.h"
#include "usbip_device_driver.h"
#include "usbip_common.h"
#include "usbip_network.h"
#include "usbip.h"

#ifndef USBIP_AS_LIBRARY
static const char usbip_disconnect_usage_string[] =
	"usbip disconnect <args>\n"
	"    -r, --remote=<host>    Address of a remote computer\n"
	"    -b, --busid=<busid>    Bus ID of a device to be disconnected\n"
	"    -d, --device           Run with an alternate driver, e.g. vUDC\n"
	;

void usbip_disconnect_usage(void)
{
	printf("usage: %s", usbip_disconnect_usage_string);
}
#endif

static int send_unexport_device(struct usbip_sock *sock,
				struct usbip_usb_device *udev)
{
	int rc;
	struct op_unexport_request request;
	struct op_unexport_reply   reply;
	uint16_t code = OP_REP_UNEXPORT;

	memset(&request, 0, sizeof(request));
	memset(&reply, 0, sizeof(reply));

	/* send a request */
	rc = usbip_net_send_op_common(sock, OP_REQ_UNEXPORT, 0);
	if (rc < 0) {
		err("send op_common");
		return -1;
	}

	memcpy(&request.udev, udev, sizeof(struct usbip_usb_device));

	PACK_OP_UNEXPORT_REQUEST(0, &request);

	rc = usbip_net_send(sock, (void *) &request, sizeof(request));
	if (rc < 0) {
		err("send op_export_request");
		return -1;
	}

	/* receive a reply */
	rc = usbip_net_recv_op_common(sock, &code);
	if (rc < 0) {
		err("recv op_common");
		return -1;
	}

	rc = usbip_net_recv(sock, (void *) &reply, sizeof(reply));
	if (rc < 0) {
		err("recv op_unexport_reply");
		return -1;
	}

	PACK_OP_EXPORT_REPLY(0, &reply);

	/* check the reply */
	if (reply.returncode) {
		err("recv error return %d", reply.returncode);
		return -1;
	}

	return 0;
}

static int unexport_device(const char *busid, struct usbip_sock *sock)
{
	struct usbip_exported_devices edevs;
	struct usbip_exported_device *edev;
	int rc;

	rc = usbip_driver_open();
	if (rc < 0) {
		err("open driver");
		goto err_out;
	}

	rc = usbip_refresh_device_list(&edevs);
	if (rc < 0) {
		err("could not refresh device list");
		goto err_driver_close;
	}

	edev = usbip_get_device(&edevs, busid);
	if (edev == NULL) {
		err("find device");
		goto err_free_edevs;
		return -1;
	}

	rc = send_unexport_device(sock, &edev->udev);
	if (rc < 0) {
		err("send unexport");
		goto err_free_edevs;
	}

	usbip_free_device_list(&edevs);
	usbip_driver_close();

	return 0;
err_free_edevs:
	usbip_free_device_list(&edevs);
err_driver_close:
	usbip_driver_close();
err_out:
	return -1;
}

int usbip_disconnect_device(const char *host, const char *port,
			    const char *busid)
{
	struct usbip_sock *sock;
	int rc;

	sock = usbip_conn_ops.open(host, port, usbip_conn_ops.opt);
	if (!sock) {
		err("tcp connect");
		return -1;
	}

	rc = unexport_device(busid, sock);
	if (rc < 0) {
		err("unexport");
		usbip_conn_ops.close(sock);
		return -1;
	}

	usbip_conn_ops.close(sock);

	if (!usbip_has_transferred()) {
		rc = usbip_unbind_device(busid);
		if (rc) {
			err("unbind");
			return -1;
		}
	}

	return 0;
}

#ifndef USBIP_AS_LIBRARY
int usbip_disconnect(int argc, char *argv[])
{
	static const struct option opts[] = {
		{ "remote", required_argument, NULL, 'r' },
		{ "busid",  required_argument, NULL, 'b' },
		{ "device", no_argument,       NULL, 'd' },
		{ NULL, 0,  NULL, 0 }
	};
	char *host = NULL;
	char *busid = NULL;
	int opt;
	int ret = -1;

	for (;;) {
		opt = getopt_long(argc, argv, "r:b:d", opts, NULL);

		if (opt == -1)
			break;

		switch (opt) {
		case 'r':
			host = optarg;
			break;
		case 'b':
			busid = optarg;
			break;
		case 'd':
			usbip_update_driver();
			break;
		default:
			goto err_out;
		}
	}

	if (!host || !busid)
		goto err_out;

	ret = usbip_disconnect_device(host, usbip_port_string, busid);
	goto out;

err_out:
	usbip_disconnect_usage();
out:
	return ret;
}
#endif /* !USBIP_AS_LIBRARY */
