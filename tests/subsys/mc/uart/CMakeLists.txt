# Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(mc_uart_test)
if(CONFIG_MC_UART_MSG_TEST)
  target_sources(app PRIVATE src/msg.c)
else()
  target_sources(app PRIVATE src/main.c)
endif()
