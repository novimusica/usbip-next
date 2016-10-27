/*
 * Copyright (C) 2016 Nobuo Iwata
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
#include <string.h>

int usbip_ex_listen(void)
{
	int listenfd;
	struct sockaddr_in addr;

	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	if (listenfd < 0) {
		perror("Failed to socket");
		goto err_out;
	}

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(EXAMPLE_PORT);

	if (bind(listenfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("Failed to bind");
		goto err_sock_close;
	}

	if (listen(listenfd, 5) < 0) {
		perror("Failed to listen");
		goto err_sock_close;
	}

	return listenfd;
err_sock_close:
	close(listenfd);
err_out:
	return -1;
}

int usbip_ex_accept(int listenfd, struct usbip_ex_net_info *info)
{
	int connfd;
	struct sockaddr_in addr;
	int addrlen = sizeof(addr);
	int reuseaddr = 1;
	int nodelay = 1;

	connfd = accept(listenfd, (struct sockaddr *)&addr, &addrlen);
	if (connfd < 0) {
		perror("Failed to accept");
		goto err_out;
	}

	if (getnameinfo((struct sockaddr *)&addr, addrlen,
			info->host, NI_MAXHOST, info->port, NI_MAXSERV,
			NI_NUMERICHOST | NI_NUMERICSERV)) {
		perror("Failed to getnameinfo");
		goto err_sock_close;
	}

	if (setsockopt(connfd, SOL_SOCKET, SO_REUSEADDR,
		       &reuseaddr, sizeof(reuseaddr))) {
		perror("Failed to set reuse addr");
		goto err_sock_close;
	}

	if (setsockopt(connfd, IPPROTO_TCP, TCP_NODELAY,
		       &nodelay, sizeof(nodelay))) {
		perror("Failed to set nodelay");
		goto err_sock_close;
	}

	return connfd;
err_sock_close:
	close(connfd);
err_out:
	return -1;
}
