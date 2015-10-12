#!/bin/sh
# Run radvd in a loop. This way we can restart qemu and do not need
# to manually restart radvd process.

if [ ! -f ./radvd.conf ]; then
    if [ ! -f $ZEPHYR_BASE/net/ip/tools/radvd.conf ]; then
	echo "Cannot find radvd.conf file."
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

while [ 1 ]; do radvd  -d 1 -C $DIR/radvd.conf -m stderr; done
