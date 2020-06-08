#
# Copyright (c) 2017, NXP
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_sources(
  soc.c
  )

zephyr_linker_sources_ifdef(CONFIG_NXP_IMX_RT_BOOT_HEADER
  ROM_START SORT_KEY 0 boot_header.ld)
