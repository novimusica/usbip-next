/*
 * Copyright (C) 2015 Nobuo Iwata
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __USBIP_UX_H
#define __USBIP_UX_H

#include <linux/list.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/socket.h>
#include <linux/atomic.h>

#define USBIP_UX_INT		3
#define USBIP_UX_TX_INT		1
#define USBIP_UX_RX_INT		2

#define USBIP_UX_SET_INT(ux)	((ux)->interrupted = USBIP_UX_INT)
#define USBIP_UX_SET_TX_INT(ux)	((ux)->interrupted |= USBIP_UX_TX_INT)
#define USBIP_UX_SET_RX_INT(ux)	((ux)->interrupted |= USBIP_UX_RX_INT)
#define USBIP_UX_IS_INT(ux)	((ux)->interrupted & USBIP_UX_INT)
#define USBIP_UX_IS_TX_INT(ux)	((ux)->interrupted & USBIP_UX_TX_INT)
#define USBIP_UX_IS_RX_INT(ux)	((ux)->interrupted & USBIP_UX_RX_INT)

#define USBIP_UX_REQ	1
#define USBIP_UX_RSP	2
#define USBIP_UX_SET_TX_REQ(ux)		((ux)->tx_flags |= USBIP_UX_REQ)
#define USBIP_UX_SET_TX_RSP(ux)		((ux)->tx_flags |= USBIP_UX_RSP)
#define USBIP_UX_SET_RX_REQ(ux)		((ux)->rx_flags |= USBIP_UX_REQ)
#define USBIP_UX_SET_RX_RSP(ux)		((ux)->rx_flags |= USBIP_UX_RSP)
#define USBIP_UX_CLEAR_TX_REQ(ux)	((ux)->tx_flags &= ~USBIP_UX_REQ)
#define USBIP_UX_CLEAR_TX_RSP(ux)	((ux)->tx_flags &= ~USBIP_UX_RSP)
#define USBIP_UX_CLEAR_RX_REQ(ux)	((ux)->rx_flags &= ~USBIP_UX_REQ)
#define USBIP_UX_CLEAR_RX_RSP(ux)	((ux)->rx_flags &= ~USBIP_UX_RSP)
#define USBIP_UX_HAS_TX_REQ(ux)		((ux)->tx_flags & USBIP_UX_REQ)
#define USBIP_UX_HAS_TX_RSP(ux)		((ux)->tx_flags & USBIP_UX_RSP)
#define USBIP_UX_HAS_RX_REQ(ux)		((ux)->rx_flags & USBIP_UX_REQ)
#define USBIP_UX_HAS_RX_RSP(ux)		((ux)->rx_flags & USBIP_UX_RSP)

struct usbip_ux {
	struct list_head node;
	int sockfd;
	struct socket *tcp_socket;
	struct semaphore lock;
	atomic_t count;
	wait_queue_head_t wait_count;
	struct usbip_device *ud;
	wait_queue_head_t wait_unlink;
	pid_t pgid;
	int interrupted;
	int tx_tout_sec;
	int tx_flags;
	int tx_error;
	int tx_bytes;
	int tx_count;
	char *tx_buf;
	wait_queue_head_t tx_req_q;
	wait_queue_head_t tx_rsp_q;
	int rx_flags;
	int rx_error;
	int rx_bytes;
	int rx_count;
	char *rx_buf;
	wait_queue_head_t rx_req_q;
	wait_queue_head_t rx_rsp_q;
};

#endif /* __USBIP_UX_H */
