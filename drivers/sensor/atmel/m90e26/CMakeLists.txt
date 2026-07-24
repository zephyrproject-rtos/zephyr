# Copyright (c) 2025 Atmel Corporation
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(m90e26.c)
zephyr_library_sources_ifdef(CONFIG_M90E26_BUS_UART m90e26_uart.c)
zephyr_library_sources_ifdef(CONFIG_M90E26_BUS_SPI m90e26_spi.c)
zephyr_library_sources_ifdef(CONFIG_M90EXX_SHELL m90e26_shell.c)
