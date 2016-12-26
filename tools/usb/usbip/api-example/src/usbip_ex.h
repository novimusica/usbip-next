/*
 * Copyright (C) 2016 Nobuo Iwata <nobuo.iwata@fujixerox.co.jp>
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

#ifndef __USBIP_API_EXAMPLE_H
#define __USBIP_API_EXAMPLE_H

#include <sys/socket.h>
#include <netdb.h>
#include <netinet/tcp.h>

#include <linux/usbip_api.h>

#define EXAMPLE_PORT 3240
#define EXAMPLE_PORT_STR "3240"

struct usbip_ex_net_info {
	char host[NI_MAXHOST];
	char port[NI_MAXSERV];
};

struct usbip_ex_net_arg {
	int connfd;
};

int usbip_ex_listen(void);
int usbip_ex_accept(int listenfd, struct usbip_ex_net_info *info);

struct usbip_sock *usbip_ex_connect(const char *host, const char *port,
				   void *opt);
void usbip_ex_close(struct usbip_sock *sock);

int usbip_ex_send(void *arg, void *buf, int len);
int usbip_ex_recv(void *arg, void *buf, int len, int wait_all);
void usbip_ex_shutdown(void *arg);

#endif /* !__USBIP_API_EXAMPLE_H */
