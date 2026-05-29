# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(fs_fcb)

FILE(GLOB app_sources src/*.c)
if(NOT CONFIG_FCB_ALLOW_FIXED_ENDMARKER)
  list(REMOVE_ITEM "src/fcb_test_crc_disabled_after_enabled.c")
endif()
target_sources(app PRIVATE ${app_sources})
target_include_directories(app PRIVATE ${ZEPHYR_BASE}/subsys/fs/fcb)
