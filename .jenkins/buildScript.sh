#!/bin/bash
cd /nfs/site/home/sys_gsrd
source /nfs/site/home/sys_gsrd/.bashrc
lsb_release -a
export PATH=/build/zephyr-sdk-0.13.1:/build/zephyrproject/.venv/bin:$PATH
export CCACHE_DIR=/build/ccache
unset CROSS_COMPILE
PROJECT_NAME="building-zephyr-project"
BOARD="intel_socfpga_agilex_socdk"
EXAMPLE="samples/hello_world"
ZEPHYR_FOLDER="zephyr"
cd /build
rm -rf /build/zephyrproject/$ZEPHYR_FOLDER
cp -rf $WORKSPACE /build/zephyrproject/$ZEPHYR_FOLDER
cd /build/zephyrproject/$ZEPHYR_FOLDER
west build -b $BOARD $EXAMPLE
