# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(uart_basic_api)

target_sources(app PRIVATE
    src/main.c
    src/test_uart_poll.c
    )
target_sources_ifdef(CONFIG_UART_INTERRUPT_DRIVEN app PRIVATE src/test_uart_fifo.c)
