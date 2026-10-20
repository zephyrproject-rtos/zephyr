# Copyright 2026 Xiaomi Corporation
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)

if(NOT DEFINED DTC_OVERLAY_FILE)
  set(DTC_OVERLAY_FILE "${CMAKE_CURRENT_SOURCE_DIR}/bsim_host_only.overlay")
endif()

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(bsim_test_hci_virt_host)

add_subdirectory("${ZEPHYR_BASE}/tests/bsim/babblekit" babblekit)
target_link_libraries(app PRIVATE babblekit)

target_sources(app PRIVATE src/main.c)

zephyr_include_directories(
  "${BSIM_COMPONENTS_PATH}/libUtilv1/src"
  "${BSIM_COMPONENTS_PATH}/libPhyComv1/src"
)
