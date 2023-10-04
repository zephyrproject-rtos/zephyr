# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(CONFIG_KINETIS_FLASH_CONFIG	flash_configuration.c)

add_subdirectory(${SOC_SERIES})

# This is for access to pinctrl macros
zephyr_include_directories(common)

zephyr_linker_sources_ifdef(CONFIG_KINETIS_FLASH_CONFIG
  ROM_START
  SORT_KEY ${CONFIG_KINETIS_FLASH_CONFIG_OFFSET}
  flash_config.ld
  )

# CMSIS SystemInit will disable watchdog unless instructed not to.
# Add a compiler definition here to leave watchdog untouched
# if this Kconfig is set
zephyr_compile_definitions_ifdef(CONFIG_WDOG_ENABLE_AT_BOOT DISABLE_WDOG=0)
