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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>

static void signal_handler(int i)
{
	usbip_break_all_connections();
}

static void set_signal(void)
{
	struct sigaction act;

	memset(&act, 0, sizeof(act));
	act.sa_handler = signal_handler;
	sigemptyset(&act.sa_mask);
	sigaction(SIGTERM, &act, NULL);
	act.sa_handler = SIG_IGN;
	sigaction(SIGCLD, &act, NULL);
}

int main(int argc, char *argv[])
{
	pid_t childpid;
	int listenfd, connfd;
	struct usbip_sock sock;
	struct usbip_ex_net_info info;
	struct usbip_ex_net_arg arg;

	set_signal();

	if (usbip_open_driver()) {
		perror("Failed to open driver");
		goto err_out;
	}

	listenfd = usbip_ex_listen();
	if (listenfd < 0)
		goto err_close_driver;

	while (1) {
		connfd = usbip_ex_accept(listenfd, &info);
		if (connfd < 0)
			goto err_sock_close;

		childpid = fork();
		if (childpid < 0) {
			perror("Failed to fork");
			goto err_conn_close;
		} else if (childpid == 0) {
			close(listenfd);
			arg.connfd = connfd;
			usbip_sock_init(&sock, connfd, &arg,
					usbip_ex_send, usbip_ex_recv,
					usbip_ex_shutdown);
			printf("processing %s:%s\n", info.host, info.port);
			usbip_recv_pdu(&sock, info.host, info.port);
			printf("end of process %s:%s\n", info.host, info.port);
			return EXIT_SUCCESS;
		}
		close(connfd);
	}

	close(listenfd);
	usbip_close_driver();
	EXIT_SUCCESS;
err_conn_close:
	close(connfd);
err_sock_close:
	close(listenfd);
err_close_driver:
	usbip_close_driver();
err_out:
	return EXIT_FAILURE;
}
