# Copyright (c) 2025 Pedro Ramos
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(atm90e26.c)
zephyr_library_sources_ifdef(CONFIG_ATM90E26_BUS_SPI atm90e26_spi.c)
zephyr_library_sources_ifdef(CONFIG_ATM90E26_BUS_UART atm90e26_uart.c)
zephyr_library_sources_ifdef(CONFIG_ATM90E26_EVENTS atm90e26_events.c)
zephyr_library_sources_ifdef(CONFIG_ATM90E26_SHELL atm90e26_shell.c)
zephyr_library_sources_ifdef(CONFIG_EMUL_ATM90E26 emul_atm90e26.c)
zephyr_include_directories_ifdef(CONFIG_EMUL_ATM90E26 .)
