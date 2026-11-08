# Copyright (c) 2025 Pedro Ramos
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(atm90e32as.c)
zephyr_library_sources_ifdef(CONFIG_ATM90E32AS_EVENTS atm90e32as_events.c)
zephyr_library_sources_ifdef(CONFIG_ATM90E32AS_SHELL atm90e32as_shell.c)
zephyr_library_sources_ifdef(CONFIG_EMUL_ATM90E32AS emul_atm90e32as.c)
zephyr_include_directories_ifdef(CONFIG_EMUL_ATM90E32AS .)
