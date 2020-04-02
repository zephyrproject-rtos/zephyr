# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)

if((BOARD STREQUAL nrf52840dk_nrf52840) OR
   (BOARD STREQUAL nrf52_pca10040) OR
   (BOARD STREQUAL nrf9160dk_nrf9160) OR
   (BOARD STREQUAL reel_board)
)
  if(EXISTS               ${APPLICATION_SOURCE_DIR}/boards/${BOARD}.conf)
    set(CONF_FILE "prj.conf mpu.conf boards/${BOARD}.conf")
  else()
    set(CONF_FILE "prj.conf mpu.conf")
  endif()
endif()

find_package(Zephyr HINTS $ENV{ZEPHYR_BASE})
project(fs_shell)



FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})
