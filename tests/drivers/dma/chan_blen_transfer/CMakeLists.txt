# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)

if(BOARD STREQUAL nucleo_f207zg OR
   BOARD STREQUAL nucleo_f401re OR
   BOARD STREQUAL nucleo_f746zg)
  set(CONF_FILE "prj.conf generic_board_stm32.conf generic_board_stm32_v1.conf")
elseif(BOARD STREQUAL nucleo_f091rc OR
       BOARD STREQUAL nucleo_f103rb OR
       BOARD STREQUAL nucleo_f302r8 OR
       BOARD STREQUAL nucleo_l073rz OR
       BOARD STREQUAL nucleo_l476rg)
  set(CONF_FILE "prj.conf generic_board_stm32.conf generic_board_stm32_v2.conf")
endif()

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(chan_blen_transfer)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})
