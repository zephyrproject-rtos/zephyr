# Copyright (c) 2021, ATL Electronics
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_include_directories(.)
zephyr_sources(soc.c)

zephyr_linker_sources_ifdef(CONFIG_SOC_FAMILY_BFLB RODATA rodata.ld)
