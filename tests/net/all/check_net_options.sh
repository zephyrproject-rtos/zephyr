#!/usr/bin/env bash
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

build_dir="$1"

if [ ! -d $build_dir ]; then
    echo "Directory $build_dir not found!"
    exit
fi

kconfig_dir=$build_dir/rst/doc/reference/kconfig

if [ ! -d $kconfig_dir ]; then
    echo "Kconfig documentation not found at $kconfig_dir"
    exit
fi

get_options()
{
    cd $kconfig_dir; ls CONFIG_DNS* CONFIG_NET* CONFIG_IEEE802154* | sed 's/\.rst//g'
}

get_options | while read opt
do
    grep -q $opt prj.conf > /dev/null 2>&1
    if [ $? -ne 0 ]; then
	echo "$opt"
    fi
done

echo "Total number of options       : `get_options | wc -w`" > /dev/tty
