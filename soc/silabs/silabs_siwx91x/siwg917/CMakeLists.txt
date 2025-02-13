# Copyright (c) 2024 Silicon Laboratories Inc.
# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(CONFIG_SOC_SERIES_SIWG917 soc.c)
zephyr_sources_ifdef(CONFIG_WISECONNECT_NETWORK_STACK nwp_init.c)

set(SOC_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld CACHE INTERNAL "")
