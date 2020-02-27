# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)

set(DTS_ROOTS ${CMAKE_CURRENT_LIST_DIR})
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(devicetree)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})
