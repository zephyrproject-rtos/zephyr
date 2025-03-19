# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

# Adding two configuration overlay files to the build instead of specifying
# them on the west command line. This is done to make it easier to build.
# This sample application require the Zephyr Bluetooth Controller to be enabled
# so that the controller stack can be modifed to test the 't'
# feature.
set(EXTRA_CONF_FILE 
    ${CMAKE_CURRENT_SOURCE_DIR}/overlay-bt_ll_sw_split.conf
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/common.conf
)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(iso_groupchat_mixer)

target_sources(app PRIVATE 
    src/main.c
    ../common/common.c
)
