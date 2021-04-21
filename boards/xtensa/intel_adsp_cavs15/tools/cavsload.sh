#!/bin/sh
# Copyright (c) 2021 Intel Corporation
# SPDX-License-Identifier: Apache-2.0
set -e

# General purpose loader tool for a remote ssh-accessible CAVS 1.5
# device (e.g. an Up Squared board running Linux). Can be used as the
# twister hook for both --device-serial-pty and --west-flash, e.g.:
#
#   twister -p intel_adsp_cavs15 \
#     --device-testing --device-serial-pty=/path/to/cavsload.sh \
#     --west-flash=/path/to/cavsload.sh
#
# Alternatively, pass a built "zephyr.elf" file (in a complete build
# tree, not just a standalone file) as the single argument and the
# script will synchronously flash the device and begin emitting its
# logs to standard output.
#
# The remote host must be accessible via non-interactive ssh access
# and the remote account must have password-free sudo ability.  (The
# intent is that isolating the host like this to be a CAVS test unit
# means that simple device access at root is acceptable.)  There must
# be a current Zephyr tree on the host, and a working loadable
# "diag_driver" kernel module.

# Remote host on which to test
HOST=up2

# Zephyr tree on the host
HOST_ZEPHYR_BASE=z/zephyr

# rimage key to use for signing binaries
KEY=$HOME/otc_private_key.pem

# Local path to a built rimage (https://github.com/thesofproject/rimage)
RIMAGE=$ZEPHYR_BASE/../modules/audio/sof/zephyr/ext/rimage

# Kernel module on host (https://github.com/thesofproject/sof-diagnostic-driver)
HOST_DRIVER=sof-diagnostic-driver/diag_driver.ko

########################################################################
#
# Twister has frustrating runtime behavior with this device.  The
# flash tool is run via west as part of the build, has a working
# directory in the build tree, and is passed the build directory as
# its command line argument.  The console/serial tool is run globally
# in $ZEPHYR_BASE.  But the console script has no arguments, and thus
# can't find the test directory!  And worse, the scripts are
# asynchronous and may start in either order, but our console script
# can't be allowed to run until after the flash.  If it does, it will
# pull old data (from the last test run!)  out of the buffer and
# confuse twister.
#
# So the solution here is to have the console script do the trace
# reading AND the flashing.  The flash script merely signs the binary
# and places it into ZEPHYR_BASE for the console script to find.  The
# console script then just has to wait for the binary to appear (which
# solves the ordering problem), flash it, delete it (so as not to
# confuse the next test run), and emit the adsplog output as expected.
#
# One side effect is that the logs for the firmware load appear in a
# separate file ("cavslog_load.log" in $ZEPHYR_BASE) and not the
# device.log file that twister expects.

if [ "$(basename $1)" = "zephyr.elf" ]; then
    # Standalone mode (argument is a path to zephyr.elf)
    BLDDIR=$(dirname $(dirname $1))
    DO_SIGN=1
    DO_LOAD=1
    DO_LOG=1
elif [ "$1" = "" ]; then
    # Twister --device-serial-pty mode
    DO_LOAD=1
    DO_LOG=1
else
    # Twister --west-flash mode
    BLDDIR=$1
    DO_SIGN=1
fi

IMAGE=$ZEPHYR_BASE/_cavstmp.ri
LOADLOG=$ZEPHYR_BASE/_cavsload_load.log
HOST_TOOLS=$HOST_ZEPHYR_BASE/boards/xtensa/intel_adsp_cavs15/tools
FWLOAD=$HOST_TOOLS/fw_loader.py
ADSPLOG=$HOST_TOOLS/adsplog.py

if [ "$DO_SIGN" = "1" ]; then
    ELF=$BLDDIR/zephyr/zephyr.elf.mod
    BOOT=$BLDDIR/zephyr/bootloader.elf.mod
    $RIMAGE/rimage -v -k $KEY -o $IMAGE -c $RIMAGE/config/apl.toml \
		   -i 3 -e $BOOT $ELF > $BLDDIR/rimage.log
fi

if [ "$DO_LOAD" = "1" ]; then
    while [ ! -e $IMAGE ]; do
	sleep 0.1
    done

    scp $IMAGE $HOST:_cavstmp.ri
    ssh $HOST "(lsmod | grep -q diag_driver) || sudo insmod $HOST_DRIVER"

    # The script sometimes gets stuck
    ssh $HOST "sudo pkill -f -9 fw_loader.py" || true
    ssh $HOST "sudo $FWLOAD -f _cavstmp.ri || true" > $LOADLOG 2>&1

    if [ "$DO_SIGN" = "1" ]; then
	cat $LOADLOG
    fi

    sleep 1
    rm -f $IMAGE
fi

if [ "$DO_LOG" = "1" ]; then
    ssh $HOST "sudo pkill -f -9 adsplog.py" || true
    ssh $HOST "sudo $ADSPLOG"
fi
