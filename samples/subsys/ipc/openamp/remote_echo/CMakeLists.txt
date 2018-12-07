cmake_minimum_required(VERSION 3.8.2)
# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#

macro(set_conf_file)
  if(EXISTS ${APPLICATION_SOURCE_DIR}/boards/${BOARD}.conf)
    set(CONF_FILE "prj.conf ${APPLICATION_SOURCE_DIR}/boards/${BOARD}.conf")
  else()
    set(CONF_FILE "prj.conf")
  endif()
endmacro()

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(openamp_remote_echo)

target_sources(app PRIVATE src/main.c)
target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
