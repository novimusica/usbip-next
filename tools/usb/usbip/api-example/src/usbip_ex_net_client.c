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

#include "usbip_ex.h"
#include <stdlib.h>
#include <string.h>

struct usbip_sock *usbip_ex_connect(const char *host, const char *port,
				   void *opt)
{
	int connfd;
	struct addrinfo hints, *rp;
	struct sockaddr_in addr;
	struct usbip_sock *sock;
	struct usbip_ex_net_arg *arg;
	int nodelay = 1;

	memset(&hints, 0, sizeof(hints));
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_family = AF_INET;

	if (getaddrinfo(host, port, &hints, &rp)) {
		perror("Failed to getaddrinfo");
		goto err_out;
	}

	connfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
	if (connfd < 0) {
		perror("Failed to socket");
		goto err_addr_free;
	}

	sock = (struct usbip_sock *)malloc(sizeof(struct usbip_sock));
	if (!sock) {
		perror("Failed to mallo sock");
		goto err_sock_close;
	}

	arg = (struct usbip_ex_net_arg *)
			malloc(sizeof(struct usbip_ex_net_arg));
	if (!arg) {
		perror("Failed to mallo arg");
		goto err_sock_free;
	}

	if (connect(connfd, rp->ai_addr, rp->ai_addrlen)) {
		perror("Failed to connect");
		goto err_arg_free;
	}

	if (setsockopt(connfd, IPPROTO_TCP, TCP_NODELAY,
		       &nodelay, sizeof(nodelay))) {
		perror("Failed to set nodelay");
		goto err_sock_close;
	}

	arg->connfd = connfd;

	usbip_sock_init(sock, connfd, arg,
			usbip_ex_send, usbip_ex_recv,
			usbip_ex_shutdown);
	freeaddrinfo(rp);
	return sock;
err_arg_free:
	free(arg);
err_sock_free:
	free(sock);
err_sock_close:
	close(connfd);
err_addr_free:
	freeaddrinfo(rp);
err_out:
	return NULL;
}

void usbip_ex_close(struct usbip_sock *sock)
{
	struct usbip_ex_net_arg *arg = (struct usbip_ex_net_arg *)sock->arg;

	close(arg->connfd);
	free(arg);
	free(sock);
}
