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
# means that simple device access at root is acceptable.)  The
# firmware load and logging scripts must be present on the device and
# match the paths below.

# Remote host on which to test
HOST=tgl2
#HOST=up2

# rimage key to use for signing binaries
KEY=$HOME/otc_private_key_3k.pem
#KEY=$HOME/otc_private_key.pem

# Firmware load script on the host (different for APL and TGL)
FWLOAD=./cavs-fw-v25.py
#FWLOAD=./cavs-fw-v15.py

# Local path to a built rimage directory (https://github.com/thesofproject/rimage)
RIMAGE=$HOME/rimage

# Zephyr tree on the host
HOST_ZEPHYR_BASE=z/zephyr

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

if [ "$1" = "" ]; then
    # Twister --device-serial-pty mode
    DO_LOAD=1
    DO_LOG=1
elif [ "$(basename $1)" = "zephyr.elf" ]; then
    # Standalone mode (argument is a path to zephyr.elf)
    BLDDIR=$(dirname $(dirname $1))
    DO_SIGN=1
    DO_LOAD=1
    DO_LOG=1
else
    # Twister --west-flash mode
    BLDDIR=$1
    DO_SIGN=1
fi

IMAGE=$ZEPHYR_BASE/_cavstmp.ri
LOADLOG=$ZEPHYR_BASE/_cavsload_load.log
ADSPLOG=./adsplog.py

if [ "$DO_SIGN" = "1" ]; then
    ELF=$BLDDIR/zephyr/zephyr.elf.mod
    BOOT=$BLDDIR/zephyr/bootloader.elf.mod
    (cd $BLDDIR; west sign --tool-data=$RIMAGE/config -t rimage -- -k $KEY)
    cp $BLDDIR/zephyr/zephyr.ri $IMAGE
fi

if [ "$DO_LOAD" = "1" ]; then
    while [ ! -e $IMAGE ]; do
	sleep 0.1
    done

    scp $IMAGE $HOST:_cavstmp.ri

    # The script sometimes gets stuck
    ssh $HOST "sudo pkill -f -9 cavs-fw-" || true
    ssh $HOST "sudo $FWLOAD _cavstmp.ri" > $LOADLOG 2>&1

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
