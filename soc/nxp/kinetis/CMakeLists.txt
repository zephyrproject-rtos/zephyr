# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(CONFIG_KINETIS_FLASH_CONFIG	flash_configuration.c)

add_subdirectory(${SOC_SERIES})

# This is for access to pinctrl macros
zephyr_include_directories(common)
zephyr_include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../common)

zephyr_linker_sources_ifdef(CONFIG_KINETIS_FLASH_CONFIG
  ROM_START
  SORT_KEY ${CONFIG_KINETIS_FLASH_CONFIG_OFFSET}
  flash_config.ld
  )

# CMSIS SystemInit will disable watchdog unless instructed not to.
# Add a compiler definition here to leave watchdog untouched
# if this Kconfig is not set
zephyr_compile_definitions_ifndef(CONFIG_WDT_DISABLE_AT_BOOT DISABLE_WDOG=1)
