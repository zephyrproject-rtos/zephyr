# SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(hci_uart_3wire)

set(sample_src "${ZEPHYR_BASE}/samples/bluetooth/hci_uart_3wire/src/main.c")

# The test runs the sample's main() in a thread of its own
set_source_files_properties("${sample_src}" PROPERTIES
  COMPILE_DEFINITIONS "main=hci_uart_3wire_main"
)

target_sources(app PRIVATE
  "${sample_src}"
  src/main.c
)
