#
# Copyright (c) 2022, NXP
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_compile_definitions(FSL_SDK_DRIVER_QUICK_ACCESS_ENABLE)

zephyr_sources(
  soc.c
  flash_clock_setup.c
  )

zephyr_library_include_directories(
  ${ZEPHYR_BASE}/kernel/include
  ${ZEPHYR_BASE}/arch/${ARCH}/include
  )

zephyr_linker_sources_ifdef(CONFIG_NXP_IMX_RT5XX_BOOT_HEADER
  ROM_START SORT_KEY 0 boot_header.ld)

zephyr_code_relocate(flash_clock_setup.c SRAM)
