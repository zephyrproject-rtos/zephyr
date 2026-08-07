# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources(retention.c)
zephyr_library_sources_ifdef(CONFIG_RETENTION_BOOT_MODE bootmode.c)

if(CONFIG_RETENTION_BOOTLOADER_INFO_TYPE_MCUBOOT)
  zephyr_library_sources(blinfo_mcuboot.c)
  zephyr_link_libraries(MCUBOOT_BOOTUTIL)
endif()
