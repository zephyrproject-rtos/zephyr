# Copyright (c) 2019, NXP
#
# SPDX-License-Identifier: Apache-2.0
#
cmake_minimum_required(VERSION 3.13.1)

set(BOARD lpcxpresso54114_m4)


include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
include(ExternalProject)

ExternalProject_Add(
  rpmsg_lite_remote
  SOURCE_DIR ${APPLICATION_SOURCE_DIR}/remote
  INSTALL_COMMAND ""      # This particular build system has no install command
)

project(NONE)

target_sources(app PRIVATE src/main_master.c)
add_dependencies(core_m0_inc_target rpmsg_lite_remote)

target_include_directories(app PRIVATE $ENV{ZEPHYR_BASE}/samples/subsys/ipc/rpmsg_lite)
target_include_directories(app PRIVATE $ENV{ZEPHYR_BASE}/drivers)
target_include_directories(app PRIVATE $ENV{ZEPHYR_BASE}/samples/subsys/ipc/rpmsg_lite/master/src)
target_include_directories(app PRIVATE $ENV{ZEPHYR_BASE}/samples/subsys/ipc/rpmsg_lite/remote/build/zephyr)

if(CONFIG_SOC_SERIES_LPC54XXX)
target_include_directories(app PRIVATE $ENV{ZEPHYR_BASE}/ext/lib/ipc/rpmsg_lite/lib/include/platform/lpc5411x)
target_sources(app PRIVATE $ENV{ZEPHYR_BASE}/ext/lib/ipc/rpmsg_lite/lib/rpmsg_lite/porting/platform/lpc5411x/rpmsg_platform_zephyr_ipm.c)
endif()

target_sources(app PRIVATE src/main_master.c)
