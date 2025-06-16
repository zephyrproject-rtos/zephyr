#
# Copyright (c) 2025 Nuvoton Technology Corporation.
#
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(
  .
  ${ZEPHYR_BASE}/drivers
)

zephyr_library_sources_ifdef(CONFIG_USBH_HID           hid_core.c)
zephyr_library_sources_ifdef(CONFIG_USBH_HID           hid_driver.c)


set(SOC_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld CACHE INTERNAL "")
# set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")

