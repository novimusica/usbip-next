#!/bin/sh

function clean_tool() {
sudo make uninstall
make clean
if [ ! -x cleanup.sh ]; then
	chmod a+x cleanup.sh
fi
./cleanup.sh
}

DIR_TEST=`pwd`
DIR_DRV=$DIR_TEST/drivers/usb/usbip
DIR_TOOL=$DIR_TEST/tools/usb/usbip
DIR_EX=$DIR_TEST/tools/usb/usbip/api-example
DIR_LIBUSB_TOOL=$DIR_TEST/tools/usb/usbip/libusb
KERNEL=`uname -r`
DIR_KERN_INC=/lib/modules/$KERNEL/build/include
DIR_KERN_MOD=/lib/modules/$KERNEL/kernel/drivers/usb/usbip
DIR_UAPI_INC=/usr/include/linux

sudo rm -f $DIR_KERN_INC/uapi/linux/usbip_ux.h
sudo rm -f $DIR_UAPI_INC/usbip_ux.h
sudo rm -f $DIR_UAPI_INC/usbip_api.h

pushd .
cd $DIR_DRV
make clean
sudo rm $DIR_KERN_MOD/*.ko
sudo depmod -a
popd

if [ -e $DIR_EX ]; then
pushd .
cd $DIR_EX
clean_tool
popd
fi

if [ -e $DIR_LIBUSB_TOOL ]; then
pushd .
cd $DIR_LIBUSB_TOOL
clean_tool
popd
fi

pushd .
cd $DIR_TOOL
clean_tool
popd
