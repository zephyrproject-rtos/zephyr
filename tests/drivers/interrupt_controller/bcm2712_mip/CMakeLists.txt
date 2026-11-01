# Copyright (c) 2026 Process Mission
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(bcm2712_mip)

target_sources(app PRIVATE
  src/mip.c
  src/routes.c
)
target_include_directories(app PRIVATE
  "${ZEPHYR_BASE}/drivers/interrupt_controller"
  "${ZEPHYR_BASE}/soc/brcm/bcm2712"
)
