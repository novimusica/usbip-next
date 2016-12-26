/*
 * Copyright (C) 2005-2007 Takahiro Hirofuchi
 */

#ifndef __VHCI_DRIVER_H
#define __VHCI_DRIVER_H

#include <libudev.h>
#include <stdint.h>

#include "usbip_common.h"

#define USBIP_VHCI_BUS_TYPE "platform"

int usbip_vhci_driver_open(void);
void usbip_vhci_driver_close(void);

int usbip_vhci_get_free_port(void);
int usbip_vhci_find_device(const char *host, const char *busid);

/* will be removed */
int usbip_vhci_attach_device(int port, int sockfd, uint8_t busnum,
			     uint8_t devnum, uint32_t speed);
int usbip_vhci_detach_device(int port);

int usbip_vhci_create_record(const char *host, const char *port,
			     const char *busid, int rhport);
int usbip_vhci_delete_record(int rhport);

int usbip_vhci_imported_devices_dump(void);

#endif /* __VHCI_DRIVER_H */
