# Top-level CMakeLists.txt for the skeleton application.
#
# Copyright (c) 2017 Open Source Foundries Limited
#
# SPDX-License-Identifier: Apache-2.0
#
# This provides a basic application structure suitable for communication using
# mcumgr.  It can be used as a starting point for new applications.

# Zephyr uses Device Tree (DT) to describe some board hardware configuration.
#
# See the Zephyr documentation for more information on DT:
# http://docs.zephyrproject.org/devices/dts/device_tree.html
set(DTC_OVERLAY_FILE "${CMAKE_CURRENT_SOURCE_DIR}/dts.overlay")

# Standard Zephyr application boilerplate.
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(NONE)

assert_exists(DTC_OVERLAY_FILE)

target_sources(app PRIVATE
    src/main.c
)

zephyr_link_libraries_ifdef(CONFIG_FILE_SYSTEM_NFFS NFFS)
zephyr_link_libraries_ifdef(CONFIG_MBEDTLS mbedTLS)
