#
# Copyright 2024 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

# Pass this flag so the SDK I2C, UART and SPI drivers do not init the LP
# Flexcomm SDK driver
zephyr_compile_definitions_ifdef(CONFIG_NXP_LP_FLEXCOMM LPFLEXCOMM_INIT_NOT_USED_IN_DRIVER=1)

zephyr_sources(soc.c)

if(CONFIG_FLASH_MCUX_FLEXSPI_NOR)
  zephyr_sources(flash_clock_setup.c)
  if(CONFIG_FLASH_MCUX_FLEXSPI_XIP)
    zephyr_code_relocate(FILES flash_clock_setup.c LOCATION ${CONFIG_FLASH_MCUX_FLEXSPI_XIP_MEM}_TEXT)
  endif()
endif()

zephyr_include_directories(.)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")
