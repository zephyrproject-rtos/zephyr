# Copyright (c) 2023 Trackunit Corporation
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

if(CONFIG_MODEM_BACKEND_TTY)
  zephyr_library_sources(modem_backend_tty.c)
  target_sources(native_simulator INTERFACE modem_backend_tty_bottom.c)
endif()
zephyr_library_sources_ifdef(CONFIG_MODEM_BACKEND_UART modem_backend_uart.c)
zephyr_library_sources_ifdef(CONFIG_MODEM_BACKEND_UART_ISR modem_backend_uart_isr.c)
if(CONFIG_MODEM_BACKEND_UART_ASYNC)
  zephyr_library_sources_ifdef(CONFIG_MODEM_BACKEND_UART_ASYNC_HWFC modem_backend_uart_async_hwfc.c)
  zephyr_library_sources_ifndef(CONFIG_MODEM_BACKEND_UART_ASYNC_HWFC modem_backend_uart_async.c)
endif()
