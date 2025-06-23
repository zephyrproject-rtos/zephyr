#
# Copyright (c) 2025 Nuvoton Technology Corporation.
#
# SPDX-License-Identifier: Apache-2.0

add_subdirectory(class)

zephyr_include_directories(
  .
  ${ZEPHYR_BASE}/drivers
)

zephyr_sources(
  hal_ohci.c
  usbh_core.c
  usbh_debug.c
  usbh_hub.c
  usbh_ohci.c
  usbh_support.c
)

set(SOC_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld CACHE INTERNAL "")
# set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")

