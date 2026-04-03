#
# Copyright 2024 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_compile_definitions_ifdef(CONFIG_NXP_LP_FLEXCOMM LPFLEXCOMM_INIT_NOT_USED_IN_DRIVER=1)
zephyr_compile_definitions_ifdef(CONFIG_UDC_DRIVER USB_DEVICE_CONFIG_BUFFER_PROPERTY_CACHEABLE=1)

zephyr_compile_definitions(FSL_SDK_DRIVER_QUICK_ACCESS_ENABLE)

zephyr_include_directories(.)

zephyr_sources(init.c)

zephyr_sources_ifdef(CONFIG_MIPI_DSI	display_if.c)

zephyr_sources_ifdef(CONFIG_MFD_PCA9422	pmic_int.c)

if(CONFIG_FLASH_MCUX_XSPI_XIP)
  zephyr_sources(flash_clock_setup.c)
  zephyr_code_relocate(FILES flash_clock_setup.c LOCATION ${CONFIG_FLASH_MCUX_XSPI_XIP_MEM}_TEXT)
endif()

zephyr_library_include_directories(
  ${ZEPHYR_BASE}/kernel/include
  ${ZEPHYR_BASE}/arch/${ARCH}/include
  )

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")
