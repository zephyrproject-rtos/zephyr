# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(bootrom)

set(app_sources src/main.c)

if(CONFIG_MPU)
  list(APPEND app_sources src/sys_wrapper.c)
endif()

target_sources(app PRIVATE ${app_sources})
