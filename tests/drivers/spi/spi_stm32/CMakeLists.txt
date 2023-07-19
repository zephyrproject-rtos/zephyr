# Copyright (c) 2023 Graphcore Ltd.
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

project(spi_stm32_test)

find_package(Zephyr COMPONENTS unittest REQUIRED HINTS $ENV{ZEPHYR_BASE})

target_sources(testbinary
  PRIVATE
    src/main.c
)
