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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <getopt.h>

#include "usbip_host_driver.h"
#include "usbip_common.h"
#include "usbip.h"

static const char usbip_bind_usage_string[] =
	"usbip bind <args>\n"
	"    -b, --busid=<busid>    Bind " USBIP_HOST_DRV_NAME ".ko to device "
	"on <busid>\n";

void usbip_bind_usage(void)
{
	printf("usage: %s", usbip_bind_usage_string);
}

int usbip_bind(int argc, char *argv[])
{
	static const struct option opts[] = {
		{ "busid", required_argument, NULL, 'b' },
		{ NULL,    0,                 NULL,  0  }
	};

	int opt;
	int ret = -1;

	for (;;) {
		opt = getopt_long(argc, argv, "b:", opts, NULL);

		if (opt == -1)
			break;

		switch (opt) {
		case 'b':
			ret = usbip_bind_device(optarg);
			goto out;
		default:
			goto err_out;
		}
	}

err_out:
	usbip_bind_usage();
out:
	return ret;
}
