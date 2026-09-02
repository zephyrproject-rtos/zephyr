# Copyright (c) 2026 CogniPilot Foundation
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(icm45686_stream)

target_include_directories(app PRIVATE
  ${ZEPHYR_BASE}/drivers/sensor/tdk/icm45686
  ${ZEPHYR_HAL_TDK_MODULE_DIR}
  ${ZEPHYR_HAL_TDK_MODULE_DIR}/common
  ${ZEPHYR_HAL_TDK_MODULE_DIR}/icm456xx
  ${ZEPHYR_HAL_TDK_MODULE_DIR}/icm456xx/icm456xx_h
)
target_sources(app PRIVATE src/main.c)
