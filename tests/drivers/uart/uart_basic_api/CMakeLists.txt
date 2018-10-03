cmake_minimum_required(VERSION 3.8.2)
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(NONE)

target_sources(app PRIVATE
    src/main.c
    src/test_uart_poll.c
    )
target_sources_ifdef(CONFIG_UART_INTERRUPT_DRIVEN app PRIVATE src/test_uart_fifo.c)
