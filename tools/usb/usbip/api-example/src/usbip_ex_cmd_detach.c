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
#include <stdio.h>

static void usage(const char *cmd)
{
	printf("%s <port>\n", cmd);
}

int main(int argc, char *argv[])
{
	if (argc < 2) {
		usage(argv[0]);
		goto err_out;
	}

	if (usbip_detach_port(argv[1])) {
		perror("Failed to detach device");
		goto err_out;
	}

	return EXIT_SUCCESS;
err_out:
	return EXIT_FAILURE;
}
