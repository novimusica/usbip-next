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

#include "usbip_common.h"
#include "usbip_host_common.h"
#include "usbip_device_driver.h"

void usbip_update_driver(void)
{
	usbip_hdriver = &device_driver;
}

int usbip_bind_device(const char *busid)
{
	if (!usbip_hdriver->ops.bind_device)
		return 0;
	return usbip_hdriver->ops.bind_device(busid);
}

int usbip_unbind_device(const char *busid)
{
	if (!usbip_hdriver->ops.unbind_device)
		return 0;
	return usbip_hdriver->ops.unbind_device(busid);
}
