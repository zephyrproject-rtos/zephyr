#
# Copyright (c) 2020, NXP
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_sources(
  soc.c
  )

zephyr_library_include_directories(
  ${ZEPHYR_BASE}/kernel/include
  ${ZEPHYR_BASE}/arch/${ARCH}/include
  )

zephyr_linker_sources_ifdef(CONFIG_NXP_IMX_RT6XX_BOOT_HEADER
  ROM_START SORT_KEY 0 boot_header.ld)

zephyr_linker_sources(
  RWDATA quick_access.ld)
