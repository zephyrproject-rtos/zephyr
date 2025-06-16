#
# Copyright (c) 2025 Nuvoton Technology Corporation.
#
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_USBH_OHCI)
  add_subdirectory(usbh)
endif()

zephyr_include_directories(
  .
  ${ZEPHYR_BASE}/drivers
)

zephyr_sources(
  soc.c
  gdma.c
  header.c
)

set(SOC_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld CACHE INTERNAL "")
# set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")
