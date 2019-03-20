#
# Copyright (c) 2018, Cypress
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_include_directories(.)
zephyr_sources(
	soc.c
)

zephyr_linker_sources_ifdef(CONFIG_SOC_SERIES_PSOC62 NOINIT noinit.ld)
zephyr_linker_sources_ifdef(CONFIG_SOC_SERIES_PSOC62 RWDATA rwdata.ld)
