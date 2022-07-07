#!/bin/sh
# Copyright (c) 2022 Intel Corporation
# SPDX-License-Identifier: Apache-2.0
set -e

# Twister integration tool for a remote ssh-accessible cAVS audio DSP
# host (e.g. an Up Squared board running Linux). Can be used as the
# hook for both --device-serial-pty and --west-flash, for example:
#
#  export CAVS_HOST=tgl2
#  export CAVS_KEY=$HOME/otc_private_key_3k.pem
#  export CAVS_RIMAGE=$HOME/rimage
#
#  twister -p intel_adsp_cavs25 --device-testing \
#     --device-serial-pty=$ZEPHYR_BASE/soc/xtensa/intel_adsp/tools/cavstwist.sh \
#     --west-flash=$ZEPHYR_BASE/soc/xtensa/intel_adsp/tools/cavstwist.sh
#
# The device at CAVS_HOST must be accessible via non-interactive ssh
# access and the remote account must have password-free sudo ability.
# (The intent is that isolating the host like this to be a CAVS test
# unit means that simple device access at root is acceptable.)
#
# The signing key must be correct for your device.  In general we want
# to be using the SOF-curated "community" keys.  Note that cAVS 2.5
# uses a different (longer) key than the earlier platforms.
#
# CAVS_RIMAGE must be the path to a current, checked out and built (!)
# rimage tool from SOF: https://github.com/thesofproject/rimage
#
# Alternatively, pass a built "zephyr.elf" file (in a complete build
# tree, not just a standalone file) as the single argument and the
# script will synchronously flash the device and begin emitting its
# logs to standard output.
#
# Implementation notes:
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
# And notice the race protection: it's possible twister will run a
# signing script IMMEDIATELY, simultaneous with the last flash script.
# This may thus clobber the version that the other script hasn't seen
# yet.  So we add an extra wait step in the signing path for the
# loader to "accept" the file.
#
########################################################################

# Three configuration variables to set.  Have to be passed via the
# environment because this script is launched from twister.

if [ -z "$CAVS_HOST" -o -z "$CAVS_KEY" -o -z "$CAVS_RIMAGE" ]; then
    echo "Missing cavstwist.sh configuration, must have:" 1>&2
    echo "  export CAVS_HOST=ssh_host_name[:port]" 1>&2
    echo "  export CAVS_KEY=/path/to/signing_key.pem" 1>&2
    echo "  export CAVS_RIMAGE=/path/to/built/rimage/dir" 1>&2
    exit 1
fi

########################################################################

CAVSTOOL=$ZEPHYR_BASE/soc/xtensa/intel_adsp/tools/cavstool.py
IMAGE=$ZEPHYR_BASE/_cavstmp.ri
IMAGE2=$ZEPHYR_BASE/_cavstmp2.ri

if [ "$1" = "" ]; then
    # Twister --device-serial-pty mode
    DO_LOAD=1
elif [ "$(basename $1)" = "zephyr.elf" ]; then
    # Standalone mode (argument is a path to zephyr.elf)
    BLDDIR=$(dirname $(dirname $1))
    DO_SIGN=1
    DO_LOAD=1
else
    # Twister --west-flash mode
    BLDDIR=$1
    DO_SIGN=1
fi

if [ "$DO_SIGN" = "1" ]; then
    # Wait for the last flasher to accept the old file
    while [ -e $IMAGE ]; do
	sleep 0.1
    done

    ELF=$BLDDIR/zephyr/zephyr.elf.mod
    BOOT=$BLDDIR/zephyr/bootloader.elf.mod
    (cd $BLDDIR;
     west sign --tool-data=$CAVS_RIMAGE/config -t rimage -- -k $CAVS_KEY)
    cp $BLDDIR/zephyr/zephyr.ri $IMAGE
fi

if [ "$DO_LOAD" = "1" ]; then
    while [ ! -e $IMAGE ]; do
	sleep 0.1
    done

    # Must "accept" the file so the next signing script knows it's OK
    # to write a new one
    mv $IMAGE $IMAGE2
    scp $IMAGE2 $CAVSTOOL scp://$CAVS_HOST
    rm -f $IMAGE2

    # Twister seems to overlap tests by a tiny bit, and of course the
    # remote system might have other junk running from manual testing.
    # If something is doing log reading at the moment of firmware
    # load, it tends fairly reliably to hang the DSP.  Kill it.
    ssh ssh://$CAVS_HOST "sudo pkill -9 -f cavstool" ||:

    exec ssh ssh://$CAVS_HOST "sudo ./$(basename $CAVSTOOL) $(basename $IMAGE2)"
fi
