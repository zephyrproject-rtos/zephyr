# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(integration_test)

target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/test_caps.c)
