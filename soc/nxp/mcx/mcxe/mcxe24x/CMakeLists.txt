# Copyright 2025 NXP
# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(CONFIG_MCXE_FLASH_CONFIG	flash_configuration.c)

zephyr_include_directories(.)

zephyr_sources(soc.c)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")

zephyr_linker_sources_ifdef(CONFIG_MCXE_FLASH_CONFIG
  ROM_START
  SORT_KEY 0x400
  flash_config.ld
  )

# CMSIS SystemInit will disable watchdog unless instructed not to.
# Add a compiler definition here to leave watchdog untouched
# if this Kconfig is set
zephyr_compile_definitions_ifdef(CONFIG_WDOG_ENABLE_AT_BOOT DISABLE_WDOG=0)
