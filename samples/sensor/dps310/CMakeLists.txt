# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)
set(DTC_OVERLAY_FILE "${CMAKE_CURRENT_SOURCE_DIR}/dps310.dts")
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)

project(dps310)
FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})
