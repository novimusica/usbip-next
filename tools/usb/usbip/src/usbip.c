/*
 * command structure borrowed from udev
 * (git://git.kernel.org/pub/scm/linux/hotplug/udev.git)
 *
 * Copyright (C) 2011 matt mooney <mfm@muteddisk.com>
 *               2005-2007 Takahiro Hirofuchi
 * Copyright (C) 2015-2016 Nobuo Iwata <nobuo.iwata@fujixerox.co.jp>
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

#include "usbip_config.h"

#include <stdio.h>
#include <stdlib.h>

#include <getopt.h>

#include "usbip_common.h"
#include "usbip_network.h"
#include "usbip.h"

static int usbip_help(int argc, char *argv[]);
static int usbip_version(int argc, char *argv[]);

static const char usbip_version_string[] = PACKAGE_STRING;

static const char usbip_usage_string[] =
	"usbip [--debug]"
#ifdef USBIP_WITH_LIBUSB
	" [--debug-flags HEX]"
#endif
	" [--log] [--tcp-port PORT] [version]\n"
	"             [help] <command> <args>\n";

static void usbip_usage(void)
{
	printf("usage: %s", usbip_usage_string);
}

struct command {
	const char *name;
	int (*fn)(int argc, char *argv[]);
	const char *help;
	void (*usage)(void);
};

static const struct command cmds[] = {
	{
		"help",
		usbip_help,
		NULL,
		NULL
	},
	{
		"version",
		usbip_version,
		NULL,
		NULL
	},
#ifndef USBIP_WITH_LIBUSB
	{
		"attach",
		usbip_attach,
		"Attach a remote USB device",
		usbip_attach_usage
	},
	{
		"detach",
		usbip_detach,
		"Detach a remote USB device",
		usbip_detach_usage
	},
#endif
	{
		"connect",
		usbip_connect,
		"Connect a USB device to a remote computer",
		usbip_connect_usage
	},
	{
		"disconnect",
		usbip_disconnect,
		"Disconnect a USB device from a remote computer",
		usbip_disconnect_usage
	},
	{
		"list",
		usbip_list,
		"List exportable or local USB devices",
		usbip_list_usage
	},
	{
		"bind",
		usbip_bind,
		"Bind device to " USBIP_HOST_DRV_NAME ".ko",
		usbip_bind_usage
	},
	{
		"unbind",
		usbip_unbind,
		"Unbind device from " USBIP_HOST_DRV_NAME ".ko",
		usbip_unbind_usage
	},
#ifndef USBIP_WITH_LIBUSB
	{
		"port",
		usbip_port_show,
		"Show imported USB devices",
		NULL
	},
#endif
	{ NULL, NULL, NULL, NULL }
};

static int usbip_help(int argc, char *argv[])
{
	const struct command *cmd;
	int i;
	int ret = 0;

	if (argc > 1 && argv++) {
		for (i = 0; cmds[i].name != NULL; i++)
			if (!strcmp(cmds[i].name, argv[0]) && cmds[i].usage) {
				cmds[i].usage();
				goto done;
			}
		ret = -1;
	}

	usbip_usage();
	printf("\n");
	for (cmd = cmds; cmd->name != NULL; cmd++)
		if (cmd->help != NULL)
			printf("  %-10s %s\n", cmd->name, cmd->help);
	printf("\n");
done:
	return ret;
}

static int usbip_version(int argc, char *argv[])
{
	(void) argc;
	(void) argv;

	printf(PROGNAME " (%s)\n", usbip_version_string);
	return 0;
}

static int run_command(const struct command *cmd, int argc, char *argv[])
{
	dbg("running command: `%s'", cmd->name);
	return cmd->fn(argc, argv);
}

int main(int argc, char *argv[])
{
	static const struct option opts[] = {
		{ "debug",    no_argument,       NULL, 'd' },
#ifdef USBIP_WITH_LIBUSB
		{ "debug-flags", required_argument, NULL, 'f' },
#endif
		{ "log",      no_argument,       NULL, 'l' },
		{ "tcp-port", required_argument, NULL, 't' },
		{ NULL,       0,                 NULL,  0  }
	};

	char *cmd;
	int opt;
	int i, rc = -1;

	usbip_use_stderr = 1;
	opterr = 0;
	for (;;) {
		opt = getopt_long(argc, argv, "+d"
#ifdef USBIP_WITH_LIBUSB
					      "f:"
#endif
					      "lt:", opts, NULL);
		if (opt == -1)
			break;

		switch (opt) {
		case 'd':
			usbip_use_debug = 1;
			break;
#ifdef USBIP_WITH_LIBUSB
		case 'f':
			usbip_debug_flag = strtoul(optarg, NULL, 16);
			break;
#endif
		case 'l':
#ifndef USBIP_OS_NO_SYSLOG
			usbip_use_syslog = 1;
			openlog("", LOG_PID, LOG_USER);
#endif
			break;
		case 't':
			usbip_setup_port_number(optarg);
			break;
		case '?':
			printf("usbip: invalid option\n");
		default:
			usbip_usage();
			goto out;
		}
	}

	cmd = argv[optind];
	if (cmd) {
		for (i = 0; cmds[i].name != NULL; i++)
			if (!strcmp(cmds[i].name, cmd)) {
				argc -= optind;
				argv += optind;
				optind = 0;
				usbip_net_tcp_conn_init();
				rc = run_command(&cmds[i], argc, argv);
				goto out;
			}
	}

	/* invalid command */
	usbip_help(0, NULL);
out:
	return (rc > -1 ? EXIT_SUCCESS : EXIT_FAILURE);
}
