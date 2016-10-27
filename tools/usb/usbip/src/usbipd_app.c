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
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>

#include "vhci_driver.h"
#include "usbip_common.h"
#include "usbip_network.h"
#include "usbipd.h"

char *usbip_progname = "usbipa";
char *usbip_default_pid_file = "/var/run/usbipa";

int usbip_open_driver(void)
{
	if (usbip_vhci_driver_open()) {
		err("please load " USBIP_CORE_MOD_NAME ".ko and "
			USBIP_VHCI_DRV_NAME ".ko!");
		return -1;
	}
	return 0;
}

void usbip_close_driver(void)
{
	usbip_vhci_driver_close();
}

static int import_device(struct usbip_sock *sock, struct usbip_usb_device *udev)
{
	int rc;
	int port;

	dbg("Sockfd:%d", sock->fd);
	port = usbip_vhci_get_free_port();
	if (port < 0) {
		err("no free port");
		return -1;
	}

	dump_usb_device(udev);
	rc = usbip_vhci_attach_device(port, sock->fd, udev->busnum,
					udev->devnum, udev->speed);
	if (rc < 0) {
		err("import device");
		return -1;
	}

	return port;
}

static int recv_request_export(struct usbip_sock *sock,
			       const char *host, const char *port)
{
	struct op_export_request req;
	struct op_export_reply reply;
	int port_nr = 0;
	int error = 0;
	int rc;

	memset(&req, 0, sizeof(req));
	memset(&reply, 0, sizeof(reply));

	rc = usbip_net_recv(sock, &req, sizeof(req));
	if (rc < 0) {
		dbg("usbip_net_recv failed: export request");
		goto err_out;
	}
	PACK_OP_EXPORT_REQUEST(0, &req);

	rc = usbip_ux_try_transfer_init(sock);
	if (rc < 0) {
		err("transfer init");
		goto err_out;
	}

	port_nr = import_device(sock, &req.udev);
	if (port_nr < 0) {
		dbg("export request busid %s: failed", req.udev.busid);
		error = 1;
	}

	rc = usbip_net_send_op_common(sock, OP_REP_EXPORT,
				      (!error ? ST_OK : ST_NA));
	if (rc < 0) {
		dbg("usbip_net_send_op_common failed: %#0x", OP_REP_EXPORT);
		goto err_try_trx_exit;
	}

	if (!error)
		reply.returncode = 0;
	else
		reply.returncode = -1;

	PACK_OP_EXPORT_REPLY(0, &rep);

	rc = usbip_net_send(sock, &reply, sizeof(reply));
	if (rc < 0) {
		dbg("usbip_net_send failed: export reply");
		goto err_try_trx_exit;
	}

	if (!error) {
		rc = usbip_vhci_create_record(host, port, req.udev.busid,
					      port_nr);
		if (rc < 0) {
			err("record connection");
			goto err_try_trx_exit;
		}
	}
	dbg("export request busid %s: complete %d", req.udev.busid, error);

	rc = usbip_ux_try_transfer(sock);
	if (rc < 0) {
		err("try transfer");
		goto err_try_trx_exit;
	}
	usbip_ux_try_transfer_exit(sock);

	return 0;
err_try_trx_exit:
	usbip_ux_try_transfer_exit(sock);
err_out:
	return -1;
}

static int unimport_device(const char *host, struct usbip_usb_device *udev)
{
	int port, rc;

	port = usbip_vhci_find_device(host, udev->busid);
	if (port < 0) {
		err("no imported port %s %s", host, udev->busid);
		return -1;
	}

	rc = usbip_vhci_detach_device(port);
	if (rc < 0) {
		err("no imported port %d %s %s", port, host, udev->busid);
		return -1;
	}
	return port;
}

static int recv_request_unexport(struct usbip_sock *sock, const char *host)
{
	struct op_unexport_request req;
	struct op_unexport_reply reply;
	int port_nr = 0;
	int error = 0;
	int rc;

	memset(&req, 0, sizeof(req));
	memset(&reply, 0, sizeof(reply));

	rc = usbip_net_recv(sock, &req, sizeof(req));
	if (rc < 0) {
		dbg("usbip_net_recv failed: unexport request");
		return -1;
	}
	PACK_OP_UNEXPORT_REQUEST(0, &req);

	port_nr = unimport_device(host, &req.udev);
	if (port_nr < 0)
		error = 1;

	rc = usbip_net_send_op_common(sock, OP_REP_UNEXPORT,
				      (!error ? ST_OK : ST_NA));
	if (rc < 0) {
		dbg("usbip_net_send_op_common failed: %#0x", OP_REP_UNEXPORT);
		return -1;
	}

	if (!error) {
		reply.returncode = 0;
	} else {
		reply.returncode = -1;
		dbg("unexport request busid %s: failed", req.udev.busid);
		return -1;
	}
	PACK_OP_UNEXPORT_REPLY(0, &rep);

	rc = usbip_net_send(sock, &reply, sizeof(reply));
	if (rc < 0) {
		dbg("usbip_net_send failed: unexport reply");
		return -1;
	}

	if (!error)
		usbip_vhci_delete_record(port_nr);

	dbg("unexport request busid %s: complete", req.udev.busid);

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
	case OP_REQ_EXPORT:
		ret = recv_request_export(sock, host, port);
		break;
	case OP_REQ_UNEXPORT:
		ret = recv_request_unexport(sock, host);
		break;
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
