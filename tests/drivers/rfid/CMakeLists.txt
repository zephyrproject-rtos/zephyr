# Copyright (c) 2025 Conny Marco Menebröcker
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

set(DTC_OVERLAY_FILE ${CMAKE_CURRENT_SOURCE_DIR}/boards/native_sim.overlay)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(device)

target_include_directories(app
  PRIVATE
    include
    mocks
)

target_sources(app
  PRIVATE
    src/cr95hf_spi.c
)
