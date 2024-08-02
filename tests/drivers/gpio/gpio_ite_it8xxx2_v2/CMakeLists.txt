# Copyright 2023 The ChromiumOS Authors
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(gpio_ite_it8xxx2_v2)

target_include_directories(app PRIVATE
  include
)

zephyr_include_directories(
  include
  ${ZEPHYR_BASE}/soc/ite/ec/common
  ${ZEPHYR_BASE}/soc/ite/ec/it8xxx2
)

target_sources(app
  PRIVATE
    src/main.c
)
