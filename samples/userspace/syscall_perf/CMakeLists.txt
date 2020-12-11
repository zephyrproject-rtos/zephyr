# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
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
