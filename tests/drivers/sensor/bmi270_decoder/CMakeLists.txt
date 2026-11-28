# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(bmi270_decoder)

target_include_directories(app PRIVATE ${ZEPHYR_BASE}/drivers/sensor/bosch/bmi270)
target_sources(app PRIVATE
  src/main.c
  ${ZEPHYR_BASE}/drivers/sensor/bosch/bmi270/bmi270_decoder.c
)
