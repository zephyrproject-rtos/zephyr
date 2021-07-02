#
# Copyright (c) 2017-2021, NXP
#
# SPDX-License-Identifier: Apache-2.0
#
zephyr_sources_ifdef(CONFIG_SOC_SERIES_IMX_RT11XX soc_rt11xx.c)
zephyr_sources_ifdef(CONFIG_SOC_SERIES_IMX_RT10XX soc_rt10xx.c)

zephyr_linker_sources_ifdef(CONFIG_NXP_IMX_RT_BOOT_HEADER
  ROM_START SORT_KEY 0 boot_header.ld)
