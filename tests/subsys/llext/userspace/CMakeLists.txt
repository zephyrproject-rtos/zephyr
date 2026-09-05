# Copyright (c) 2023 Intel Corporation.
# Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)

# The application-defined syscall of the shared test sources lives
# outside this application directory, so it is not picked up by the
# CONFIG_APPLICATION_DEFINED_SYSCALL scan and must be added here.
list(APPEND SYSCALL_INCLUDE_DIRS ${CMAKE_CURRENT_LIST_DIR}/../src)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(llext_userspace_test)

include(${CMAKE_CURRENT_LIST_DIR}/../llext_tests.cmake)
