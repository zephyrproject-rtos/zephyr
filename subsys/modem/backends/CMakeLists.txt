# Copyright (c) 2023 Trackunit Corporation
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_MODEM_BACKEND_TTY modem_backend_tty.c)
zephyr_library_sources_ifdef(CONFIG_MODEM_BACKEND_UART modem_backend_uart.c)
zephyr_library_sources_ifdef(CONFIG_MODEM_BACKEND_UART_ISR modem_backend_uart_isr.c)
zephyr_library_sources_ifdef(CONFIG_MODEM_BACKEND_UART_ASYNC modem_backend_uart_async.c)
