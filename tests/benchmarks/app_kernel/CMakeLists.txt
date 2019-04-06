# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)
if(BOARD STREQUAL "minnowboard")
  set(CONF_FILE prj_fp.conf)
endif()

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(app_kernel)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})
