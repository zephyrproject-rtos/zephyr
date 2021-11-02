# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(log_core_additional)

if(${USERSPACE_TEST})
target_sources(app PRIVATE src/log_test_user.c)
else()
target_sources(app PRIVATE src/log_test.c)
endif()
