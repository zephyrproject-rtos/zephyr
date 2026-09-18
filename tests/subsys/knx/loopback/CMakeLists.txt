# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2026 Fabien Proriol

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(knx_loopback)

# The loopback driver is compiled for real (it has no devicetree dependency).
# CONFIG_KNX_STACK stays off so knx_core.c's SYS_INIT does not start the stack.
target_sources(app PRIVATE
    src/main.c
    ${ZEPHYR_BASE}/drivers/knx/knx_loopback.c
)
target_compile_definitions(app PRIVATE CONFIG_KNX_STACK_LOG_LEVEL=0)
