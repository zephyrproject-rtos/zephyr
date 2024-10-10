# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources(storage_area.c)
# zephyr-keep-sorted-start
zephyr_library_sources_ifdef(CONFIG_STORAGE_AREA_DISK storage_area_disk.c)
zephyr_library_sources_ifdef(CONFIG_STORAGE_AREA_EEPROM storage_area_eeprom.c)
zephyr_library_sources_ifdef(CONFIG_STORAGE_AREA_FLASH storage_area_flash.c)
zephyr_library_sources_ifdef(CONFIG_STORAGE_AREA_RAM storage_area_ram.c)
zephyr_library_sources_ifdef(CONFIG_STORAGE_AREA_STORE storage_area_store.c)
# zephyr-keep-sorted-stop
