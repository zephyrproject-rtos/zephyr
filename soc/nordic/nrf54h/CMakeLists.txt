# Copyright (c) 2024 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_SOC_NRF54H20_ENGA_CPUAPP OR CONFIG_SOC_NRF54H20_ENGA_CPURAD)
  zephyr_include_directories(.)
  zephyr_library_sources(soc.c)
endif()

# Ensure that image size aligns with 16 bytes so that MRAMC finalizes all writes
# for the image correctly
zephyr_linker_sources(SECTIONS SORT_KEY zzz_place_align_at_end align.ld)
