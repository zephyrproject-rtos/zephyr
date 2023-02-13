#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(protected_storage)

target_sources(app PRIVATE src/main.c)

target_include_directories(app PRIVATE
  $<TARGET_PROPERTY:tfm,TFM_BINARY_DIR>/install/interface/include
)
