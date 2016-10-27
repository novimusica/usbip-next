/*
 * Copyright (C) 2015 Nobuo Iwata
 *               2011 matt mooney <mfm@muteddisk.com>
 *               2005-2007 Takahiro Hirofuchi
 * Copyright (C) 2015-2016 Samsung Electronics
 *               Igor Kotrasinski <i.kotrasinsk@samsung.com>
 *               Krzysztof Opasiak <k.opasiak@samsung.com>
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
#include <unistd.h>
#include <errno.h>

#include "vhci_driver.h"
#include "usbip_common.h"
#include "usbip_network.h"
#include "usbip.h"

#ifndef USBIP_AS_LIBRARY
static const char usbip_attach_usage_string[] =
	"usbip attach <args>\n"
	"    -r, --remote=<host>      The machine with exported USB devices\n"
	"    -b, --busid=<busid>    Busid of the device on <host>\n"
	"    -d, --device=<devid>    Id of the virtual UDC on <host>\n";

void usbip_attach_usage(void)
{
	printf("usage: %s", usbip_attach_usage_string);
}
#endif

static int import_device(struct usbip_sock *sock, struct usbip_usb_device *udev)
{
	int rc;
	int port;

	rc = usbip_vhci_driver_open();
	if (rc < 0) {
		err("open vhci_driver");
		return -1;
	}

	port = usbip_vhci_get_free_port();
	if (port < 0) {
		err("no free port");
		usbip_vhci_driver_close();
		return -1;
	}

	rc = usbip_vhci_attach_device(port, sock->fd, udev->busnum,
				      udev->devnum, udev->speed);
	if (rc < 0) {
		err("import device");
		usbip_vhci_driver_close();
		return -1;
	}

	usbip_vhci_driver_close();

	return port;
}

static int query_import_device(struct usbip_sock *sock, const char *busid)
{
	int rc;
	struct op_import_request request;
	struct op_import_reply   reply;
	uint16_t code = OP_REP_IMPORT;

	memset(&request, 0, sizeof(request));
	memset(&reply, 0, sizeof(reply));

	/* send a request */
	rc = usbip_net_send_op_common(sock, OP_REQ_IMPORT, 0);
	if (rc < 0) {
		err("send op_common");
		return -1;
	}

	strncpy(request.busid, busid, SYSFS_BUS_ID_SIZE-1);

	PACK_OP_IMPORT_REQUEST(0, &request);

	rc = usbip_net_send(sock, (void *) &request, sizeof(request));
	if (rc < 0) {
		err("send op_import_request");
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
		err("recv op_import_reply");
		return -1;
	}

	PACK_OP_IMPORT_REPLY(0, &reply);

	/* check the reply */
	if (strncmp(reply.udev.busid, busid, SYSFS_BUS_ID_SIZE)) {
		err("recv different busid %s", reply.udev.busid);
		return -1;
	}

	/* import a device */
	return import_device(sock, &reply.udev);
}

int usbip_attach_device(const char *host, const char *port, const char *busid)
{
	struct usbip_sock *sock;
	int rc;
	int port_nr;

	sock = usbip_conn_ops.open(host, port, usbip_conn_ops.opt);
	if (!sock) {
		err("tcp connect");
		goto err_out;
	}

	rc = usbip_ux_try_transfer_init(sock);
	if (rc < 0) {
		err("transfer init");
		goto err_tcp_close;
	}

	port_nr = query_import_device(sock, busid);
	if (port_nr < 0) {
		err("query");
		goto err_try_trx_exit;
	}

	rc = usbip_vhci_create_record(host, port, busid, port_nr);
	if (rc < 0) {
		err("record connection");
		goto err_try_trx_exit;
	}

	rc = usbip_ux_try_transfer(sock);
	if (rc < 0) {
		err("transfer");
		goto err_try_trx_exit;
	}

	usbip_ux_try_transfer_exit(sock);
	usbip_conn_ops.close(sock);

	return 0;
err_try_trx_exit:
	usbip_ux_try_transfer_exit(sock);
err_tcp_close:
	usbip_conn_ops.close(sock);
err_out:
	return -1;
}

#ifndef USBIP_AS_LIBRARY
int usbip_attach(int argc, char *argv[])
{
	static const struct option opts[] = {
		{ "remote", required_argument, NULL, 'r' },
		{ "busid",  required_argument, NULL, 'b' },
		{ "device",  required_argument, NULL, 'd' },
		{ NULL, 0,  NULL, 0 }
	};
	char *host = NULL;
	char *busid = NULL;
	int opt;
	int ret = -1;

	for (;;) {
		opt = getopt_long(argc, argv, "d:r:b:", opts, NULL);

		if (opt == -1)
			break;

		switch (opt) {
		case 'r':
			host = optarg;
			break;
		case 'd':
		case 'b':
			busid = optarg;
			break;
		default:
			goto err_out;
		}
	}

	if (!host || !busid)
		goto err_out;

	ret = usbip_attach_device(host, usbip_port_string, busid);
	goto out;

err_out:
	usbip_attach_usage();
out:
	return ret;
}
#endif /* !USBIP_AS_LIBRARY */
