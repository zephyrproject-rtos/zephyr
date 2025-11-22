# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(uvc_uvb)

# This sample cannot use the common USB sample infrastructure as we do not want
# to initialize the board default `zephyr_udc0` but instead an unrelated `virtual_udc0`.
#include(${ZEPHYR_BASE}/samples/subsys/usb/common/common.cmake)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})
zephyr_include_directories(${ZEPHYR_BASE}/subsys/usb/host)
