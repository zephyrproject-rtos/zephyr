# Copyright (c) 2024 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources(
  soc.c
  ../validate_rram_partitions.c)

if (CONFIG_ELV_GRTC_LFXO_ALLOWED)
  message(WARNING "WARNING! ELV mode feature is EXPERIMENTAL and may brick your device!")
endif()
