# Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(mc_comm_stress)

target_sources(app PRIVATE
  src/main.c
)

if(CONFIG_TEST_MC_STRESS_UART)
  target_sources(app PRIVATE src/transport_uart.c)
else()
  target_sources(app PRIVATE src/transport_native.c)
endif()
