# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.8.2)
macro(set_conf_file)
  if(EXISTS               ${APPLICATION_SOURCE_DIR}/boards/${BOARD}.conf)
    set(CONF_FILE "prj.conf ${APPLICATION_SOURCE_DIR}/boards/${BOARD}.conf")
  else()
    set(CONF_FILE "prj.conf")
  endif()
endmacro()

find_package(Zephyr HINTS $ENV{ZEPHYR_BASE})
project(sensor_shell)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})
