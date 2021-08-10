# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(uart_basic_api)

target_sources(app PRIVATE
    src/main.c
    src/test_uart_config.c
    src/test_uart_poll.c
    )
target_sources_ifdef(CONFIG_UART_INTERRUPT_DRIVEN app PRIVATE src/test_uart_fifo.c)
target_sources_ifdef(CONFIG_UART_INTERRUPT_DRIVEN app PRIVATE src/test_uart_pending.c)
