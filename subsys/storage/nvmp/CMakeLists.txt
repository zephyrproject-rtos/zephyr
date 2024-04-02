# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources(nvmp.c)
zephyr_library_sources_ifdef(CONFIG_NVMP_EEPROM nvmp_eeprom.c)
zephyr_library_sources_ifdef(CONFIG_NVMP_FLASH nvmp_flash.c)
