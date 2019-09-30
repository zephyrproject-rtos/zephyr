# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)

if(BOARD STREQUAL "96b_argonkey")
  set(CONF_FILE "prj.conf 96b_argonkey.conf")
else()
  set(CONF_FILE prj.conf)
endif()

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(i2s_api)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})
