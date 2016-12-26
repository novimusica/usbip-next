/*
 * Copyright (C) 2011 matt mooney <mfm@muteddisk.com>
 *               2005-2007 Takahiro Hirofuchi
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

int driver_open(void)
{
	if (usbip_vhci_driver_open()) {
		err("please load " USBIP_CORE_MOD_NAME ".ko and "
			USBIP_VHCI_DRV_NAME ".ko!");
		return -1;
	}
	return 0;
}

void driver_close(void)
{
	usbip_vhci_driver_close();
}

struct usbipd_driver_ops usbipd_driver_ops = {
	.open = driver_open,
	.close = driver_close,
};

static int import_device(struct usbip_sock *sock,
			 struct usbip_usb_device *udev,
			 const char *host, const char *port, const char *busid,
			 uint32_t *status)
{
	int rc;
	int port_nr;

	dbg("Sockfd:%d", sock->fd);
	dump_usb_device(udev);

	do {
		port_nr = usbip_vhci_get_free_port();
		if (port_nr < 0) {
			err("no free port");
			*status = ST_NO_FREE_PORT;
			goto err_out;
		}

		rc = usbip_vhci_attach_device(port_nr, sock->fd, udev->busnum,
					      udev->devnum, udev->speed);
		if (rc < 0 && errno != EBUSY) {
			err("import device");
			*status = ST_NA;
			goto err_out;
		}
	} while (rc < 0);

	rc = usbip_vhci_create_record(host, port, busid, port_nr);
	if (rc < 0) {
		err("record connection");
		*status = ST_NA;
		goto err_detach_device;
	}

	return 0;

err_detach_device:
	usbip_vhci_detach_device(port_nr);
err_out:
	return -1;
}

static int recv_request_export(struct usbip_sock *sock,
			       const char *host, const char *port)
{
	struct op_export_request req;
	uint32_t status = ST_OK;
	int rc;

	memset(&req, 0, sizeof(req));

	rc = usbip_net_recv(sock, &req, sizeof(req));
	if (rc < 0) {
		dbg("usbip_net_recv failed: export request");
		return -1;
	}
	PACK_OP_EXPORT_REQUEST(0, &req);

	rc = import_device(sock, &req.udev, host, port, req.udev.busid,
			   &status);
	if (rc < 0)
		dbg("export request busid %s: failed", req.udev.busid);

	rc = usbip_net_send_op_common(sock, OP_REP_EXPORT, status);
	if (rc < 0) {
		dbg("usbip_net_send_op_common failed: %#0x", OP_REP_EXPORT);
		return -1;
	}

	dbg("export request busid %s: complete %u", req.udev.busid, status);

	rc = usbip_ux_try_transfer(sock);
	if (rc < 0) {
		dbg("usbip_ux_try_transfer failed");
		return -1;
	}

	return 0;
}

static int unimport_device(const char *host, struct usbip_usb_device *udev,
			   uint32_t *status)
{
	int port_nr, rc;

	port_nr = usbip_vhci_find_device(host, udev->busid);
	if (port_nr < 0) {
		err("no imported port %s %s", host, udev->busid);
		*status = ST_DEVICE_NOT_FOUND;
		return -1;
	}

	rc = usbip_vhci_detach_device(port_nr);
	if (rc < 0) {
		err("no imported port %d %s %s", port_nr, host, udev->busid);
		*status = ST_NA;
		return -1;
	}

	usbip_vhci_delete_record(port_nr);

	return 0;
}

static int recv_request_unexport(struct usbip_sock *sock,
				 const char *host, const char *port)
{
	struct op_unexport_request req;
	uint32_t status = ST_OK;
	int rc;

	(void)port;

	memset(&req, 0, sizeof(req));

	rc = usbip_net_recv(sock, &req, sizeof(req));
	if (rc < 0) {
		dbg("usbip_net_recv failed: unexport request");
		return -1;
	}
	PACK_OP_UNEXPORT_REQUEST(0, &req);

	rc = unimport_device(host, &req.udev, &status);
	if (rc < 0)
		dbg("unexport request busid %s: failed", req.udev.busid);

	rc = usbip_net_send_op_common(sock, OP_REP_UNEXPORT, status);
	if (rc < 0) {
		dbg("usbip_net_send_op_common failed: %#0x", OP_REP_UNEXPORT);
		return -1;
	}

	dbg("unexport request busid %s: complete %u", req.udev.busid, status);

	return 0;
}

struct usbipd_recv_pdu_op usbipd_recv_pdu_ops[] = {
	{OP_REQ_EXPORT, recv_request_export},
	{OP_REQ_UNEXPORT, recv_request_unexport},
	{OP_UNSPEC, NULL}
};
