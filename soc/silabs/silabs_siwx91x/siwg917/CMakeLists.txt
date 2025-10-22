# Copyright (c) 2024 Silicon Laboratories Inc.
# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(CONFIG_SOC_SERIES_SIWG917 soc.c)
zephyr_sources_ifdef(CONFIG_SILABS_SIWX91X_NWP siwx91x_nwp.c)
zephyr_sources_ifdef(CONFIG_SOC_SIWX91X_PM_BACKEND_PMGR soc_siwx91x_power_pmgr.c)

set(SOC_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld CACHE INTERNAL "")
