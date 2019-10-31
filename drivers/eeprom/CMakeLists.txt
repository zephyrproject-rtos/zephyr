# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_USERSPACE eeprom_handlers.c)
zephyr_library_sources_ifdef(CONFIG_EEPROM_SHELL eeprom_shell.c)
