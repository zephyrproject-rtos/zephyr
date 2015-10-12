#!/bin/sh
# Run tunslip in a loop. This way we can restart qemu and do not need
# to manually restart tunslip process that creates the tun0 device.

if [ ! -f ./tunslip6 ]; then
    if [ ! -f $ZEPHYR_BASE/net/ip/tools/tunslip6 ]; then
	echo "Cannot find tunslip6 executable."
	exit 1
    fi
    DIR=$ZEPHYR_BASE/net/ip/tools
else
    DIR=.
fi

if [ `id -u` != 0 ]; then
    echo "Run this script as a root user!"
    exit 2
fi

while [ 1 ]; do $DIR/tunslip6 -s `readlink /tmp/slip.dev` 2001:db8::1/64; done
