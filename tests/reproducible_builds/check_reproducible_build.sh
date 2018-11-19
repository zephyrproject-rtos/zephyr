#!/usr/bin/env bash
#
# Copyright (c) 2018 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

if [ -d "/tmp/zephyr_src1" ]; then
    rm -rf /tmp/zephyr_src1/
fi

mkdir /tmp/zephyr_src1
echo "Cloning latest code in zephyr_src1"
cd /tmp/zephyr_src1/
git clone https://github.com/zephyrproject-rtos/zephyr.git

cd /tmp/zephyr_src1/zephyr/
source /tmp/zephyr_src1/zephyr/zephyr-env.sh

cd samples/hello_world/
if [ -d "build" ]; then
    rm -rf build
fi

mkdir build && cd build
cmake -GNinja -DBOARD=qemu_x86 ../
ninja

if [ ! -d "/tmp/zephyr_src1/temp" ]
then
    mkdir /tmp/zephyr_src1/temp
fi

# copy zephyr.elf into a temp folder
cp /tmp/zephyr_src1/zephyr/samples/hello_world/build/zephyr/zephyr.elf \
   /tmp/zephyr_src1/temp/


cd /tmp/zephyr_src1/zephyr/samples/hello_world/
if [ -d "build" ]; then
    rm -rf build
fi

mkdir build && cd build
cmake -GNinja -DBOARD=qemu_x86 ../
ninja

if [ ! -d "/tmp/zephyr_src1/temp1" ]
then
    mkdir /tmp/zephyr_src1/temp1/
fi

# copy zephyr.elf into a temp folder
cp /tmp/zephyr_src1/zephyr/samples/hello_world/build/zephyr/zephyr.elf \
   /tmp/zephyr_src1/temp1/

# adding CONFIG_TIMER_RANDOM_GENERATOR into prj.conf
cd /tmp/zephyr_src1/zephyr/samples/hello_world/
echo 'CONFIG_TIMER_RANDOM_GENERATOR=y' > temp_prj.conf
cat prj.conf >> temp_prj.conf
mv temp_prj.conf prj.conf

if [ -d "build" ]; then
    rm -rf build
fi
mkdir build && cd build
cmake -GNinja -DBOARD=qemu_x86 ..
ninja

cd /tmp/zephyr_src1/zephyr/tests/kernel/init/
if [ -d "build" ]; then
    rm -rf build
fi
mkdir build && cd build
cmake -GNinja -DBOARD=qemu_x86 ..
ninja

if [ -f "/tmp/out.txt" ]; then
    rm -rf out.txt
fi

echo "checking reproducible build from the same directory"
python /tmp/zephyr_src1/zephyr/scripts/check_reproducible_builds.py -i1 \
    /tmp/zephyr_src1/temp/zephyr.elf -i2 /tmp/zephyr_src1/temp1/zephyr.elf

if [ -f "/tmp/out.txt" ]; then
    rm -rf out.txt
fi

echo "checking reproducible build from the different directory"
python /tmp/zephyr_src1/zephyr/scripts/check_reproducible_builds.py -i1 \
    /tmp/zephyr_src1/zephyr/tests/kernel/init/build/zephyr/zephyr.elf -i2 \
    /tmp/zephyr_src1/temp1/zephyr.elf

if [ -f "/tmp/out.txt" ]; then
    rm -rf out.txt
fi

echo "checking reproducible build with CONFIG_TIMER_RANDOM_GENERATOR"
python /tmp/zephyr_src1/zephyr/scripts/check_reproducible_builds.py -i1 \
    /tmp/zephyr_src1/zephyr/samples/hello_world/build/zephyr/zephyr.elf -i2 \
    /tmp/zephyr_src1/temp/zephyr.elf

#Delete latest cloned code
rm -rf /tmp/zephyr_src1/
#Delete the output file generated using diffoscope
if [ -f "/tmp/out.txt" ]; then
    rm -rf out.txt
fi
