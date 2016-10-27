/*
 * Copyright (C) 2015 Nobuo Iwata
 *
 * USB/IP URB transmission in userspace.
 */

#ifndef __USBIP_UX_H
#define __USBIP_UX_H

#include <pthread.h>
#include <linux/usbip_ux.h>

struct usbip_ux {
	struct usbip_sock *sock;
	int devfd;
	int started;
	pthread_t tx, rx;
	struct usbip_ux_kaddr kaddr;
};

struct usbip_sock;

int usbip_ux_try_transfer_init(struct usbip_sock *sock);
int usbip_ux_try_transfer(struct usbip_sock *sock);
void usbip_ux_try_transfer_exit(struct usbip_sock *sock);
void usbip_ux_interrupt(struct usbip_ux *ux);
void usbip_ux_interrupt_pgrp(void);
int usbip_ux_installed(void);

#endif /* !__USBIP_UX_H */
