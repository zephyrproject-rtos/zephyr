# Copyright (c) 2026 Beken Corporation
# SPDX-License-Identifier: Apache-2.0
#
# BK7258 SoC sources.
#
# zephyr_include_directories(.) is what puts this directory on the include
# path; nothing else does it, the HWMv2 SoC metadata (soc.yml) only
# registers the directory as a configure dependency. Both soc.c and the
# arch layer's cmsis_core_m.h (which includes <soc.h>) need it, and
# without it the build stops with "soc.h: No such file or directory".

zephyr_include_directories(.)
zephyr_sources(soc.c)
zephyr_sources_ifdef(CONFIG_SOC_BK7258_IPC_HEARTBEAT soc_ipc.c)

# ---------------------------------------------------------------------------
# Linker script
#
# The generic LINKER_SCRIPT choice looks for
# CONFIG_HAVE_CUSTOM_LINKER_SCRIPT, then BOARD_LINKER_SCRIPT, then
# SOC_LINKER_SCRIPT, and fails configuration when none of them is set.
# The linker template Zephyr ships for the architecture is enough here: the
# devicetree already declares zephyr,flash (the XIP window) and zephyr,sram,
# and the template lays the image out from those two regions.
# ---------------------------------------------------------------------------
set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")
