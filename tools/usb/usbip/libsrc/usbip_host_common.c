/*
 * Copyright (C) 2015-2016 Samsung Electronics
 *               Igor Kotrasinski <i.kotrasinsk@samsung.com>
 *               Krzysztof Opasiak <k.opasiak@samsung.com>
 *
 * Refactored from usbip_host_driver.c, which is:
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <errno.h>
#include <unistd.h>

#include <libudev.h>

#include "usbip_common.h"
#include "usbip_host_common.h"
#include "usbip_ux.h"
#include "list.h"
#include "sysfs_utils.h"

struct udev *udev_context;

static int32_t read_attr_usbip_status(struct usbip_usb_device *udev)
{
	char status_attr_path[SYSFS_PATH_MAX];
	int fd;
	int length;
	char status;
	int value = 0;

	snprintf(status_attr_path, SYSFS_PATH_MAX, "%s/usbip_status",
		 udev->path);

	fd = open(status_attr_path, O_RDONLY);
	if (fd < 0) {
		err("error opening attribute %s", status_attr_path);
		return -1;
	}

	length = read(fd, &status, 1);
	if (length < 0) {
		err("error reading attribute %s", status_attr_path);
		close(fd);
		return -1;
	}

	value = atoi(&status);

	return value;
}

static
struct usbip_exported_device *usbip_exported_device_new(const char *sdevpath)
{
	struct usbip_exported_device *edev = NULL;
	struct usbip_exported_device *edev_old;
	size_t size;
	int i;

	edev = calloc(1, sizeof(struct usbip_exported_device));

	edev->sudev =
		udev_device_new_from_syspath(udev_context, sdevpath);
	if (!edev->sudev) {
		err("udev_device_new_from_syspath: %s", sdevpath);
		goto err;
	}

	if (usbip_hdriver->ops.read_device(edev->sudev, &edev->udev) < 0)
		goto err;

	edev->status = read_attr_usbip_status(&edev->udev);
	if (edev->status < 0)
		goto err;

	/* reallocate buffer to include usb interface data */
	size = sizeof(struct usbip_exported_device) +
		edev->udev.bNumInterfaces * sizeof(struct usbip_usb_interface);

	edev_old = edev;
	edev = realloc(edev, size);
	if (!edev) {
		edev = edev_old;
		dbg("realloc failed");
		goto err;
	}

	for (i = 0; i < edev->udev.bNumInterfaces; i++) {
		/* vudc does not support reading interfaces */
		if (!usbip_hdriver->ops.read_interface)
			break;
		usbip_hdriver->ops.read_interface(&edev->udev, i,
						  &edev->uinf[i]);
	}

	return edev;
err:
	if (edev->sudev)
		udev_device_unref(edev->sudev);
	if (edev)
		free(edev);

	return NULL;
}

static int refresh_exported_devices(struct usbip_exported_devices *edevs)
{
	struct usbip_exported_device *edev;
	struct udev_enumerate *enumerate;
	struct udev_list_entry *devices, *dev_list_entry;
	struct udev_device *dev;
	const char *path;

	enumerate = udev_enumerate_new(udev_context);
	udev_enumerate_add_match_subsystem(enumerate,
					   usbip_hdriver->udev_subsystem);
	udev_enumerate_scan_devices(enumerate);

	devices = udev_enumerate_get_list_entry(enumerate);

	udev_list_entry_foreach(dev_list_entry, devices) {
		path = udev_list_entry_get_name(dev_list_entry);
		dev = udev_device_new_from_syspath(udev_context,
						   path);
		if (dev == NULL)
			continue;

		/* Check whether device uses usbip driver. */
		if (usbip_hdriver->ops.is_my_device(dev)) {
			edev = usbip_exported_device_new(path);
			if (!edev) {
				dbg("usbip_exported_device_new failed");
				continue;
			}

			list_add(&edev->node, &edevs->edev_list);
			edevs->ndevs++;
		}
	}

	return 0;
}

int usbip_generic_driver_open(void)
{
	udev_context = udev_new();
	if (!udev_context) {
		err("udev_new failed");
		return -1;
	}

	return 0;
}

void usbip_generic_driver_close(void)
{
	udev_unref(udev_context);
}

int usbip_generic_refresh_device_list(struct usbip_exported_devices *edevs)
{
	int rc;

	edevs->ndevs = 0;
	INIT_LIST_HEAD(&edevs->edev_list);

	rc = refresh_exported_devices(edevs);
	if (rc < 0)
		return -1;

	return 0;
}

void usbip_generic_free_device_list(struct usbip_exported_devices *edevs)
{
	struct list_head *i, *tmp;
	struct usbip_exported_device *edev;

	list_for_each_safe(i, tmp, &edevs->edev_list) {
		edev = list_entry(i, struct usbip_exported_device, node);
		list_del(i);
		free(edev);
	}
}

struct usbip_exported_device *usbip_generic_get_device(
		struct usbip_exported_devices *edevs, const char *busid)
{
	struct list_head *i;
	struct usbip_exported_device *edev;

	list_for_each(i, &edevs->edev_list) {
		edev = list_entry(i, struct usbip_exported_device, node);
		if (!strncmp(busid, edev->udev.busid, SYSFS_BUS_ID_SIZE))
			return edev;
	}

	return NULL;
}

int usbip_generic_list_devices(struct usbip_usb_device **udevs)
{
	struct udev_enumerate *enumerate;
	struct udev_list_entry *devices, *dev_list_entry;
	struct udev_device *dev;
	const char *path;
	const char *idVendor;
	const char *idProduct;
	const char *bConfValue;
	const char *bNumIntfs;
	const char *busid;
	int num, i;

	/* Create libudev device enumeration. */
	enumerate = udev_enumerate_new(udev_context);

	/* Take only USB devices that are not hubs and do not have
	 * the bInterfaceNumber attribute, i.e. are not interfaces.
	 */
	udev_enumerate_add_match_subsystem(enumerate, "usb");
	udev_enumerate_add_nomatch_sysattr(enumerate, "bDeviceClass", "09");
	udev_enumerate_add_nomatch_sysattr(enumerate, "bInterfaceNumber", NULL);
	udev_enumerate_scan_devices(enumerate);

	devices = udev_enumerate_get_list_entry(enumerate);

	num = 0;
	udev_list_entry_foreach(dev_list_entry, devices)
		num++;

	*udevs = (struct usbip_usb_device *)calloc(num,
					sizeof(struct usbip_usb_device));
	if (*udevs == NULL) {
		err("alloc udev list");
		goto err_out;
	}

	i = 0;

	/* Show information about each device. */
	udev_list_entry_foreach(dev_list_entry, devices) {
		path = udev_list_entry_get_name(dev_list_entry);
		dev = udev_device_new_from_syspath(udev_context, path);

		/* Get device information. */
		idVendor = udev_device_get_sysattr_value(dev, "idVendor");
		idProduct = udev_device_get_sysattr_value(dev, "idProduct");
		bConfValue = udev_device_get_sysattr_value(dev,
				"bConfigurationValue");
		bNumIntfs = udev_device_get_sysattr_value(dev,
				"bNumInterfaces");
		busid = udev_device_get_sysname(dev);
		if (!idVendor || !idProduct || !bConfValue || !bNumIntfs) {
			err("problem getting device attributes: %s",
			    strerror(errno));
			udev_device_unref(dev);
			goto err_free_udevs;
		}

		(*udevs + i)->idVendor = strtol(idVendor, NULL, 16),
		(*udevs + i)->idProduct = strtol(idProduct, NULL, 16);
		strncpy((*udevs + i)->busid, busid, SYSFS_BUS_ID_SIZE);

		udev_device_unref(dev);
		i++;
	}

	udev_enumerate_unref(enumerate);
	return i;

err_free_udevs:
	free(*udevs);
err_out:
	udev_enumerate_unref(enumerate);

	return -1;
}

static int modify_match_busid(const char *busid, int add)
{
	char attr_name[] = "match_busid";
	char command[SYSFS_BUS_ID_SIZE + 4];
	char match_busid_attr_path[SYSFS_PATH_MAX];
	int rc;

	snprintf(match_busid_attr_path, sizeof(match_busid_attr_path),
		 "%s/%s/%s/%s/%s/%s", SYSFS_MNT_PATH, SYSFS_BUS_NAME,
		 SYSFS_BUS_TYPE, SYSFS_DRIVERS_NAME, USBIP_HOST_DRV_NAME,
		 attr_name);

	if (add)
		snprintf(command, SYSFS_BUS_ID_SIZE + 4, "add %s", busid);
	else
		snprintf(command, SYSFS_BUS_ID_SIZE + 4, "del %s", busid);

	rc = write_sysfs_attribute(match_busid_attr_path, command,
				   sizeof(command));
	if (rc < 0) {
		dbg("failed to write match_busid: %s", strerror(errno));
		return -1;
	}

	return 0;
}

enum unbind_status {
	UNBIND_ST_OK,
	UNBIND_ST_USBIP_HOST,
	UNBIND_ST_FAILED
};

/* call at unbound state */
static int bind_usbip(const char *busid)
{
	char attr_name[] = "bind";
	char bind_attr_path[SYSFS_PATH_MAX];
	int rc = -1;

	snprintf(bind_attr_path, sizeof(bind_attr_path), "%s/%s/%s/%s/%s/%s",
		 SYSFS_MNT_PATH, SYSFS_BUS_NAME, SYSFS_BUS_TYPE,
		 SYSFS_DRIVERS_NAME, USBIP_HOST_DRV_NAME, attr_name);

	rc = write_sysfs_attribute(bind_attr_path, busid, strlen(busid));
	if (rc < 0) {
		err("error binding device %s to driver: %s", busid,
		    strerror(errno));
		return -1;
	}

	return 0;
}

/* buggy driver may cause dead lock */
static int unbind_other(const char *busid)
{
	enum unbind_status status = UNBIND_ST_OK;

	char attr_name[] = "unbind";
	char unbind_attr_path[SYSFS_PATH_MAX];
	int rc = -1;

	struct udev *udev;
	struct udev_device *dev;
	const char *driver;
	const char *bDevClass;

	/* Create libudev context. */
	udev = udev_new();

	/* Get the device. */
	dev = udev_device_new_from_subsystem_sysname(udev, "usb", busid);
	if (!dev) {
		dbg("unable to find device with bus ID %s", busid);
		goto err_close_busid_dev;
	}

	/* Check what kind of device it is. */
	bDevClass  = udev_device_get_sysattr_value(dev, "bDeviceClass");
	if (!bDevClass) {
		dbg("unable to get bDevClass device attribute");
		goto err_close_busid_dev;
	}

	if (!strncmp(bDevClass, "09", strlen(bDevClass))) {
		dbg("skip unbinding of hub");
		goto err_close_busid_dev;
	}

	/* Get the device driver. */
	driver = udev_device_get_driver(dev);
	if (!driver) {
		/* No driver bound to this device. */
		goto out;
	}

	if (!strncmp(USBIP_HOST_DRV_NAME, driver,
				strlen(USBIP_HOST_DRV_NAME))) {
		/* Already bound to usbip-host. */
		status = UNBIND_ST_USBIP_HOST;
		goto out;
	}

	/* Unbind device from driver. */
	snprintf(unbind_attr_path, sizeof(unbind_attr_path),
		 "%s/%s/%s/%s/%s/%s",
		 SYSFS_MNT_PATH, SYSFS_BUS_NAME, SYSFS_BUS_TYPE,
		 SYSFS_DRIVERS_NAME, driver, attr_name);

	rc = write_sysfs_attribute(unbind_attr_path, busid, strlen(busid));
	if (rc < 0) {
		err("error unbinding device %s from driver", busid);
		goto err_close_busid_dev;
	}

	goto out;

err_close_busid_dev:
	status = UNBIND_ST_FAILED;
out:
	udev_device_unref(dev);
	udev_unref(udev);

	return status;
}

int usbip_generic_bind_device(const char *busid)
{
	int rc;
	struct udev *udev;
	struct udev_device *dev;

	/* Check whether the device with this bus ID exists. */
	udev = udev_new();
	dev = udev_device_new_from_subsystem_sysname(udev, "usb", busid);
	if (!dev) {
		err("device with the specified bus ID does not exist");
		return -1;
	}
	udev_unref(udev);

	rc = unbind_other(busid);
	if (rc == UNBIND_ST_FAILED) {
		err("could not unbind driver from device on busid %s", busid);
		return -1;
	} else if (rc == UNBIND_ST_USBIP_HOST) {
		err("device on busid %s is already bound to %s", busid,
		    USBIP_HOST_DRV_NAME);
		return -1;
	}

	rc = modify_match_busid(busid, 1);
	if (rc < 0) {
		err("unable to bind device on %s", busid);
		return -1;
	}

	rc = bind_usbip(busid);
	if (rc < 0) {
		err("could not bind device to %s", USBIP_HOST_DRV_NAME);
		modify_match_busid(busid, 0);
		return -1;
	}

	info("bind device on busid %s: complete", busid);

	return 0;
}

int usbip_generic_unbind_device(const char *busid)
{
	char bus_type[] = "usb";
	int rc, ret = -1;

	char unbind_attr_name[] = "unbind";
	char unbind_attr_path[SYSFS_PATH_MAX];
	char rebind_attr_name[] = "rebind";
	char rebind_attr_path[SYSFS_PATH_MAX];

	struct udev *udev;
	struct udev_device *dev;
	const char *driver;

	/* Create libudev context. */
	udev = udev_new();

	/* Check whether the device with this bus ID exists. */
	dev = udev_device_new_from_subsystem_sysname(udev, "usb", busid);
	if (!dev) {
		err("device with the specified bus ID does not exist");
		goto err_close_udev;
	}

	/* Check whether the device is using usbip-host driver. */
	driver = udev_device_get_driver(dev);
	if (!driver || strcmp(driver, "usbip-host")) {
		err("device is not bound to usbip-host driver");
		goto err_close_udev;
	}

	/* Unbind device from driver. */
	snprintf(unbind_attr_path, sizeof(unbind_attr_path),
		 "%s/%s/%s/%s/%s/%s",
		 SYSFS_MNT_PATH, SYSFS_BUS_NAME, bus_type, SYSFS_DRIVERS_NAME,
		 USBIP_HOST_DRV_NAME, unbind_attr_name);

	rc = write_sysfs_attribute(unbind_attr_path, busid, strlen(busid));
	if (rc < 0) {
		err("error unbinding device %s from driver", busid);
		goto err_close_udev;
	}

	/* Notify driver of unbind. */
	rc = modify_match_busid(busid, 0);
	if (rc < 0) {
		err("unable to unbind device on %s", busid);
		goto err_close_udev;
	}

	/* Trigger new probing. */
	snprintf(rebind_attr_path, sizeof(unbind_attr_path),
		 "%s/%s/%s/%s/%s/%s",
		 SYSFS_MNT_PATH, SYSFS_BUS_NAME, bus_type, SYSFS_DRIVERS_NAME,
		 USBIP_HOST_DRV_NAME, rebind_attr_name);

	rc = write_sysfs_attribute(rebind_attr_path, busid, strlen(busid));
	if (rc < 0) {
		err("error rebinding");
		goto err_close_udev;
	}

	ret = 0;
	info("unbind device on busid %s: complete", busid);

err_close_udev:
	udev_device_unref(dev);
	udev_unref(udev);

	return ret;
}

int usbip_generic_export_device(struct usbip_exported_device *edev,
				struct usbip_sock *sock)
{
	char attr_name[] = "usbip_sockfd";
	char sockfd_attr_path[SYSFS_PATH_MAX];
	char sockfd_buff[30];
	int ret;

	if (edev->status != SDEV_ST_AVAILABLE) {
		dbg("device not available: %s", edev->udev.busid);
		switch (edev->status) {
		case SDEV_ST_ERROR:
			dbg("status SDEV_ST_ERROR");
			break;
		case SDEV_ST_USED:
			dbg("status SDEV_ST_USED");
			break;
		default:
			dbg("status unknown: 0x%x", edev->status);
		}
		return -1;
	}

	/* only the first interface is true */
	snprintf(sockfd_attr_path, sizeof(sockfd_attr_path), "%s/%s",
		 edev->udev.path, attr_name);

	snprintf(sockfd_buff, sizeof(sockfd_buff), "%d\n", sock->fd);

	ret = write_sysfs_attribute(sockfd_attr_path, sockfd_buff,
				    strlen(sockfd_buff));
	if (ret < 0) {
		err("write_sysfs_attribute failed: sockfd %s to %s",
		    sockfd_buff, sockfd_attr_path);
		return ret;
	}

	info("connect: %s", edev->udev.busid);

	return ret;
}

int usbip_generic_try_transfer(struct usbip_exported_device *edev,
			       struct usbip_sock *sock)
{
	if (!edev)
		return -1;

	return usbip_ux_try_transfer(sock);
}

int usbip_generic_has_transferred(void)
{
	return usbip_ux_installed();
}
