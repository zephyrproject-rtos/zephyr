#!/bin/bash
#
# Copyright (c) 2019 Intel Corporation.
#
# SPDX-License-Identifier: Apache-2.0

if [ -z "$1" ]; then
    echo "Usage: $0 <doc build directory>"
    echo
    echo "The parameter needs to point to a directory where Zephyr html"
    echo "documentation is generated."
    echo "Typically this is $ZEPHYR_BASE/doc/_build"
    echo
    echo "This script will generate a list of networking related Kconfig options"
    echo "that are missing from prj.conf file."
    exit
fi

BUILD_DIR="$1"

if [ ! -d $BUILD_DIR ]; then
    echo "Directory $BUILD_DIR not found!"
    exit
fi

KCONFIG_DIR=$BUILD_DIR/rst/doc/reference/kconfig

if [ ! -d $KCONFIG_DIR ]; then
    echo "Kconfig documentation not found at $KCONFIG_DIR"
    exit
fi

get_options()
{
    cd $KCONFIG_DIR; ls CONFIG_DNS* CONFIG_NET* CONFIG_IEEE802154* | sed 's/\.rst//g'
}

get_options | while read opt
do
    grep -q $opt prj.conf > /dev/null 2>&1
    if [ $? -ne 0 ]; then
	echo "$opt"
    fi
done

echo "Total number of options       : `get_options | wc -w`" > /dev/tty
