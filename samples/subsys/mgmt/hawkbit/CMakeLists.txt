# Copyright (c) 2020 Linumiz
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

# This application has its own Kconfig options.
set(KCONFIG_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/Kconfig)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(hawkbit)

zephyr_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_sources(app PRIVATE src/main.c)
target_sources_ifdef(CONFIG_NET_DHCPV4 app PRIVATE src/dhcp.c)
target_sources_ifdef(CONFIG_HAWKBIT_CUSTOM_CONFIG_DATA app PRIVATE src/custom_attributes.c)

include($ENV{ZEPHYR_BASE}/samples/net/common/common.cmake)

set(gen_dir ${ZEPHYR_BINARY_DIR}/include/generated/)

generate_inc_file_for_target(
    app
    src/self_sign.der
    ${gen_dir}/self_sign.der.inc
    )
