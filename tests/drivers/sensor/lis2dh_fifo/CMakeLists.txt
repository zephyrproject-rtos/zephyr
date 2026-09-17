# Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS "$ENV{ZEPHYR_BASE}")
project(lis2dh_fifo)

target_include_directories(app PRIVATE "${ZEPHYR_BASE}/drivers/sensor/st/lis2dh")
target_sources(app PRIVATE
  src/emul.c
  src/main.c
)
