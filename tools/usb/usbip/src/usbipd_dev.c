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

#include "usbip_config.h"

#define _GNU_SOURCE

#include <errno.h>

#ifndef USBIP_OS_NO_SYS_SOCKET
#include <netdb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#endif

#include "usbip_host_driver.h"
#include "usbip_common.h"
#include "usbip_network.h"
#include "list.h"

char *usbip_progname = "usbipd";
char *usbip_default_pid_file = "/var/run/usbipd";

int usbip_open_driver(void)
{
	return usbip_driver_open();
}

void usbip_close_driver(void)
{
	usbip_driver_close();
}

static int recv_request_import(struct usbip_sock *sock)
{
	struct usbip_exported_devices edevs;
	struct usbip_exported_device *edev;
	struct op_import_request req;
	struct op_common reply;
	struct usbip_usb_device pdu_udev;
	int found = 0;
	int error = 0;
	int rc;

	rc = usbip_refresh_device_list(&edevs);
	if (rc < 0) {
		dbg("could not refresh device list: %d", rc);
		goto err_out;
	}

	memset(&req, 0, sizeof(req));
	memset(&reply, 0, sizeof(reply));

	rc = usbip_net_recv(sock, &req, sizeof(req));
	if (rc < 0) {
		dbg("usbip_net_recv failed: import request");
		goto err_free_edevs;
	}
	PACK_OP_IMPORT_REQUEST(0, &req);

	rc = usbip_try_transfer_init(sock);
	if (rc < 0) {
		err("transfer init");
		goto err_free_edevs;
	}

	edev = usbip_get_device(&edevs, req.busid);
	if (edev) {
		info("found requested device: %s", req.busid);
		found = 1;
	}

	if (found) {
		/* export device needs a TCP/IP socket descriptor */
		rc = usbip_export_device(edev, sock);
		if (rc < 0)
			error = 1;
	} else {
		info("requested device not found: %s", req.busid);
		error = 1;
	}

	rc = usbip_net_send_op_common(sock, OP_REP_IMPORT,
				      (!error ? ST_OK : ST_NA));
	if (rc < 0) {
		dbg("usbip_net_send_op_common failed: %#0x", OP_REP_IMPORT);
		goto err_try_trx_exit;
	}

	if (error) {
		dbg("import request busid %s: failed", req.busid);
		goto err_try_trx_exit;
	}

	memcpy(&pdu_udev, &edev->udev, sizeof(pdu_udev));
	usbip_net_pack_usb_device(1, &pdu_udev);

	rc = usbip_net_send(sock, &pdu_udev, sizeof(pdu_udev));
	if (rc < 0) {
		dbg("usbip_net_send failed: devinfo");
		goto err_try_trx_exit;
	}

	dbg("import request busid %s: complete", req.busid);

	rc = usbip_try_transfer(edev, sock);
	if (rc < 0) {
		err("try transfer");
		goto err_try_trx_exit;
	}

	usbip_try_transfer_exit(sock);
	usbip_free_device_list(&edevs);

	return 0;
err_try_trx_exit:
	usbip_try_transfer_exit(sock);
err_free_edevs:
	usbip_free_device_list(&edevs);
err_out:
	return -1;
}

static int send_reply_devlist(struct usbip_sock *sock)
{
	struct usbip_exported_devices edevs;
	struct usbip_exported_device *edev;
	struct usbip_usb_device pdu_udev;
	struct usbip_usb_interface pdu_uinf;
	struct op_devlist_reply reply;
	struct list_head *j;
	int rc, i;

	rc = usbip_refresh_device_list(&edevs);
	if (rc < 0) {
		dbg("could not refresh device list: %d", rc);
		goto err_out;
	}

	reply.ndev = edevs.ndevs;
	info("exportable devices: %d", reply.ndev);

	rc = usbip_net_send_op_common(sock, OP_REP_DEVLIST, ST_OK);
	if (rc < 0) {
		dbg("usbip_net_send_op_common failed: %#0x", OP_REP_DEVLIST);
		goto err_free_edevs;
	}
	PACK_OP_DEVLIST_REPLY(1, &reply);

	rc = usbip_net_send(sock, &reply, sizeof(reply));
	if (rc < 0) {
		dbg("usbip_net_send failed: %#0x", OP_REP_DEVLIST);
		goto err_free_edevs;
	}

	list_for_each(j, &edevs.edev_list) {
		edev = list_entry(j, struct usbip_exported_device, node);
		dump_usb_device(&edev->udev);
		memcpy(&pdu_udev, &edev->udev, sizeof(pdu_udev));
		usbip_net_pack_usb_device(1, &pdu_udev);

		rc = usbip_net_send(sock, &pdu_udev, sizeof(pdu_udev));
		if (rc < 0) {
			dbg("usbip_net_send failed: pdu_udev");
			goto err_free_edevs;
		}

		for (i = 0; i < edev->udev.bNumInterfaces; i++) {
			dump_usb_interface(&edev->uinf[i]);
			memcpy(&pdu_uinf, &edev->uinf[i], sizeof(pdu_uinf));
			usbip_net_pack_usb_interface(1, &pdu_uinf);

			rc = usbip_net_send(sock, &pdu_uinf,
					sizeof(pdu_uinf));
			if (rc < 0) {
				err("usbip_net_send failed: pdu_uinf");
				goto err_free_edevs;
			}
		}
	}

	usbip_free_device_list(&edevs);

	return 0;
err_free_edevs:
	usbip_free_device_list(&edevs);
err_out:
	return -1;
}

static int recv_request_devlist(struct usbip_sock *sock)
{
#ifndef USBIP_OS_NO_EMPTY_STRUCT
	struct op_devlist_request req;
#endif
	int rc;

#ifndef USBIP_OS_NO_EMPTY_STRUCT
	memset(&req, 0, sizeof(req));

	rc = usbip_net_recv(sock, &req, sizeof(req));
	if (rc < 0) {
		dbg("usbip_net_recv failed: devlist request");
		return -1;
	}
#endif

	rc = send_reply_devlist(sock);
	if (rc < 0) {
		dbg("send_reply_devlist failed");
		return -1;
	}

	return 0;
}

int usbip_recv_pdu(struct usbip_sock *sock, const char *host, const char *port)
{
	uint16_t code = OP_UNSPEC;
	int ret;

	ret = usbip_net_recv_op_common(sock, &code);
	if (ret < 0) {
		dbg("could not receive opcode: %#0x", code);
		return -1;
	}

	info("received request: %#0x(%d)", code, sock->fd);
	switch (code) {
	case OP_REQ_DEVLIST:
		ret = recv_request_devlist(sock);
		break;
	case OP_REQ_IMPORT:
		ret = recv_request_import(sock);
		break;
	case OP_REQ_DEVINFO:
	case OP_REQ_CRYPKEY:
	default:
		err("received an unknown opcode: %#0x", code);
		ret = -1;
	}

	if (ret == 0)
		info("request %#0x(%s:%s): complete", code, host, port);
	else
		info("request %#0x(%s:%s): failed", code, host, port);

	return ret;
}
