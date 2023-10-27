# Copyright (c) 2021 IoT.bzh
# SPDX-License-Identifier: Apache-2.0

zephyr_sources(
  soc.c
)
zephyr_library_sources_ifdef(CONFIG_SOC_R8A77951 pfc_r8a77951.c)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_a_r/scripts/linker.ld CACHE INTERNAL "")
