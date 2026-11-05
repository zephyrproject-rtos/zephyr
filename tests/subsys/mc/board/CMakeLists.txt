# Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(mc_board_test)

target_sources(app PRIVATE
  ../console_prefix.c
  ../uart/src/main.c
  ../uart/src/msg.c
  ../comm_stress/src/main.c
  ../comm_stress/src/transport_uart.c
)
