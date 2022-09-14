# Copyright (c) 2023 IoT.bzh
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)
zephyr_library_sources_ifdef(CONFIG_SOC_R8A779F0 pfc_r8a779f0.c)
set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_a_r/scripts/linker.ld CACHE INTERNAL "")
