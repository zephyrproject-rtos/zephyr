# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_USERSPACE eeprom_handlers.c)
zephyr_library_sources_ifdef(CONFIG_EEPROM_SHELL eeprom_shell.c)

zephyr_library_sources_ifdef(CONFIG_EEPROM_NATIVE_POSIX eeprom_native_posix.c)
zephyr_library_sources_ifdef(CONFIG_EEPROM_AT2X eeprom_at2x.c)
zephyr_library_sources_ifdef(CONFIG_EEPROM_STM32 eeprom_stm32.c)
