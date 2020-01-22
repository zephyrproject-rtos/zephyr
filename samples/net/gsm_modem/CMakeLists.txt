# SPDX-License-Identifier: Apache-2.0

# DTS overlay files must include full path name
#set(DTC_OVERLAY_FILE "${CMAKE_CURRENT_SOURCE_DIR}/sam_e70_xplained.overlay")

cmake_minimum_required(VERSION 3.13.1)

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(fona808)

target_sources(app PRIVATE src/main.c)
