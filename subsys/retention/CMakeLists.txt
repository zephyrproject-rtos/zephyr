# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources(retention.c)
zephyr_library_sources_ifdef(CONFIG_RETENTION_BOOT_MODE bootmode.c)
zephyr_library_sources_ifdef(CONFIG_RETENTION_BOOTLOADER_CONFIG bootloader.c)

if(CONFIG_RETENTION_BOOTLOADER_CONFIG)
  zephyr_link_libraries(MCUBOOT_BOOTUTIL)
endif()
