# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(common_kernel_footprint)

FILE(GLOB app_sources src/[^up]*.c)
target_sources(app PRIVATE ${app_sources})
target_sources_ifdef(CONFIG_USERSPACE app PRIVATE src/userspace.c)
target_sources_ifdef(CONFIG_PM_DEVICE app PRIVATE src/pm_device.c)

target_include_directories(app PRIVATE
  ${ZEPHYR_BASE}/kernel/include
  ${ZEPHYR_BASE}/arch/${ARCH}/include
  )
