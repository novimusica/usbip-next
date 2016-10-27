#
# Made from Makefile.org by nobuo.iwata@fujixerox.co.jp
#

ccflags-y += -DDEBUG

obj-m += usbip-core.o
usbip-core-y := usbip_common.o usbip_event.o

obj-m += usbip-ux.o
usbip-ux-y := usbip_ux.o

obj-m += vhci-hcd.o
vhci-hcd-y := vhci_sysfs.o vhci_tx.o vhci_rx.o vhci_hcd.o

obj-m += usbip-host.o
usbip-host-y := stub_dev.o stub_main.o stub_rx.o stub_tx.o

# obj-m += usbip-vudc.o
# usbip-vudc-y := vudc_dev.o vudc_sysfs.o vudc_tx.o vudc_rx.o vudc_transfer.o vudc_main.o

# ccflags-y := -I$(src)/../../../include

KERNELDIR ?= /lib/modules/$(shell uname -r)/build
PWD       := $(shell pwd)

modules:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) LDDINC=$(PWD)/../include modules \
		C=1 CF=-D__CHECK_ENDIAN__

all:
	@for mod in $(MODULES) ; do \
		(make MODULE=$$mod) ;\
	done

clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions

depend .depend dep:
	$(CC) $(CFLAGS) -M *.c > .depend

ifeq (.depend,$(wildcard .depend))
include .depend
endif
