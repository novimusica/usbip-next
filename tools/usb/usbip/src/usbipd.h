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

#ifndef __USBIPD_H
#define __USBIPD_H

#include "usbip_common.h"

#ifdef USBIP_WITH_LIBUSB
extern unsigned long usbip_debug_flag;
#endif

extern char *usbip_progname;
extern char *usbip_default_pid_file;

int usbip_recv_pdu(struct usbip_sock *sock, const char *host, const char *port);

void usbip_update_driver(void);
int usbip_open_driver(void);
void usbip_close_driver(void);

#endif /* __USBIPD_H */
