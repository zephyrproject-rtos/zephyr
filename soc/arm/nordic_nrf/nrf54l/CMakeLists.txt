# Copyright (c) 2024 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

zephyr_sources(
  soc.c
  ../validate_rram_partitions.c)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")

if (CONFIG_ELV_GRTC_LFXO_ALLOWED)
  message(WARNING "WARNING! ELV mode feature is EXPERIMENTAL and may brick your device!")
endif()
