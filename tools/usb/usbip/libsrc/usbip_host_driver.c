/*
 * Copyright (C) 2011 matt mooney <mfm@muteddisk.com>
 *               2005-2007 Takahiro Hirofuchi
 * Copyright (C) 2015-2016 Samsung Electronics
 *               Igor Kotrasinski <i.kotrasinsk@samsung.com>
 *               Krzysztof Opasiak <k.opasiak@samsung.com>
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

#include <unistd.h>
#include <libudev.h>

#include "usbip_host_common.h"
#include "usbip_host_driver.h"

#undef  PROGNAME
#define PROGNAME "libusbip"

static int is_my_device(struct udev_device *dev)
{
	const char *driver;

	driver = udev_device_get_driver(dev);
	return driver != NULL && !strcmp(driver, USBIP_HOST_DRV_NAME);
}

static int usbip_host_driver_open(void)
{
	int ret;

	ret = usbip_generic_driver_open();
	if (ret)
		err("please load " USBIP_CORE_MOD_NAME ".ko and "
		    USBIP_HOST_DRV_NAME ".ko!");
	return ret;
}

struct usbip_host_driver host_driver = {
	.udev_subsystem = "usb",
	.ops = {
		.open = usbip_host_driver_open,
		.close = usbip_generic_driver_close,
		.refresh_device_list = usbip_generic_refresh_device_list,
		.free_device_list = usbip_generic_free_device_list,
		.get_device = usbip_generic_get_device,
		.list_devices = usbip_generic_list_devices,
		.bind_device = usbip_generic_bind_device,
		.unbind_device = usbip_generic_unbind_device,
		.export_device = usbip_generic_export_device,
		.try_transfer_init = usbip_generic_try_transfer_init,
		.try_transfer = usbip_generic_try_transfer,
		.try_transfer_exit = usbip_generic_try_transfer_exit,
		.has_transferred = usbip_generic_has_transferred,
		.read_device = read_usb_device,
		.read_interface = read_usb_interface,
		.is_my_device = is_my_device,
	},
};

struct usbip_host_driver *usbip_hdriver = &host_driver;
