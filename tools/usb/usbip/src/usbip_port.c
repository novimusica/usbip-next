/*
 * Copyright (C) 2011 matt mooney <mfm@muteddisk.com>
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
 */

#include "vhci_driver.h"
#include "usbip.h"

int usbip_list_imported_devices(void)
{
	int ret;

	if (usbip_names_init(USBIDS_FILE))
		err("failed to open %s", USBIDS_FILE);

	ret = usbip_vhci_driver_open();
	if (ret < 0) {
		err("open vhci driver");
		goto err_names_free;
	}

	printf("Imported USB devices\n");
	printf("====================\n");

	ret = usbip_vhci_imported_devices_dump();
	if (ret < 0) {
		err("dump vhci devices");
		goto err_driver_close;
	}

	usbip_vhci_driver_close();
	usbip_names_free();

	return ret;

err_driver_close:
	usbip_vhci_driver_close();
err_names_free:
	usbip_names_free();
	return -1;
}

#ifndef USBIP_AS_LIBRARY
int usbip_port_show(__attribute__((unused)) int argc,
		    __attribute__((unused)) char *argv[])
{
	int ret;

	ret = usbip_list_imported_devices();
	if (ret < 0)
		err("list imported devices");

	return ret;
}
#endif
