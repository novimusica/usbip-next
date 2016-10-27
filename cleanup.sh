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
DIR_KERNEL=/lib/modules/$KERNEL/build

sudo rm -f $DIR_KERNEL/include/uapi/linux/usbip_ux.h

pushd .
cd $DIR_DRV
make clean
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
