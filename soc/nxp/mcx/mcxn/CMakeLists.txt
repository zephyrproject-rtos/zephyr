#
# Copyright 2024-2025 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

# Pass this flag so the SDK I2C, UART and SPI drivers do not init the LP
# Flexcomm SDK driver
zephyr_compile_definitions_ifdef(CONFIG_NXP_LP_FLEXCOMM LPFLEXCOMM_INIT_NOT_USED_IN_DRIVER=1)

zephyr_linker_sources_ifdef(CONFIG_NXP_FLEXSPI_BOOT_HEADER
  ROM_START SORT_KEY 0 boot_header.ld)

if(CONFIG_NXP_FLEXSPI_BOOT_HEADER)
  zephyr_linker_section_configure(
    SECTION .rom_start
    INPUT ".flexspi_fcb"
    OFFSET ${CONFIG_FLEXSPI_CONFIG_BLOCK_OFFSET}
    KEEP
    PRIO 10
  )
  zephyr_compile_definitions(XIP_EXTERNAL_FLASH)
endif()

zephyr_sources(soc.c)

if(CONFIG_FLASH_MCUX_FLEXSPI_XIP OR CONFIG_FLASH_MCUX_FLEXSPI_NOR)
  zephyr_sources(flash_clock_setup.c)
  if(CONFIG_FLASH_MCUX_FLEXSPI_XIP)
    zephyr_code_relocate(FILES flash_clock_setup.c LOCATION ${CONFIG_FLASH_MCUX_FLEXSPI_XIP_MEM}_TEXT)
  endif()
endif()

zephyr_include_directories(.)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")
