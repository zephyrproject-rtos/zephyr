# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(uart_native_tty)

target_sources(app PRIVATE src/main.c)
target_include_directories(app PRIVATE ${ZEPHYR_BASE}/drivers/serial)

# The driver's host side is tested on its own, against pseudo terminals the
# suite creates, so that no driver instance needs a real serial port.
target_sources(native_simulator INTERFACE
  ${ZEPHYR_BASE}/drivers/serial/uart_native_tty_bottom.c
  src/pty_bottom.c
)
