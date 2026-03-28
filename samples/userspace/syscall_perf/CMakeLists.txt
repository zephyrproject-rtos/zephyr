# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(syscall_perf)

set_property(
   SOURCE src/test_user.c
   PROPERTY COMPILE_DEFINITIONS
     __ZEPHYR_USER__
)

set_property(
   SOURCE src/test_supervisor.c
   PROPERTY COMPILE_DEFINITIONS
     __ZEPHYR_SUPERVISOR__
)

target_sources(app PRIVATE src/main.c src/test_user.c src/test_supervisor.c)
