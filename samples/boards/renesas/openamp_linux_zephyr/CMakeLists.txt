cmake_minimum_required(VERSION 3.20.0)
# Copyright (c) 2025 Renesas Electronics Corporation
#
# SPDX-License-Identifier: Apache-2.0
#

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(openamp_linux_zephyr)

# METAL_MAX_DEVICE_REGIONS is used to give the number of memory regions shared
# between processors. By default only one region is defined for the vrings
# and rpmsg buffers. The METAL_MAX_DEVICE_REGIONS has to be redefined to add a
# second region for the resource table.
zephyr_compile_definitions(METAL_MAX_DEVICE_REGIONS=2)

target_include_directories(app PRIVATE ${LIBMETAL_INCLUDE_DIR} ${OPENAMP_INCLUDE_DIR} ${PLATFORM_DIR})
target_include_directories(app PRIVATE src)

target_sources(app PRIVATE src/resource_table.c src/main_remote.c)
