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

int usbip_ex_send(void *_arg, void *buf, int len)
{
	struct usbip_ex_net_arg *arg = (struct usbip_ex_net_arg *)_arg;

	return send(arg->connfd, buf, len, 0);
}

int usbip_ex_recv(void *_arg, void *buf, int len, int wait_all)
{
	struct usbip_ex_net_arg *arg = (struct usbip_ex_net_arg *)_arg;

	return recv(arg->connfd, buf, len, wait_all ? MSG_WAITALL : 0);
}

void usbip_ex_shutdown(void *_arg)
{
	struct usbip_ex_net_arg *arg = (struct usbip_ex_net_arg *)_arg;

	shutdown(arg->connfd, SHUT_RDWR);
}
