# Copyright (c) 2024 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_ARM)
  zephyr_library_sources(soc.c)
endif()

if(CONFIG_NRFX_GPPI AND NOT CONFIG_NRFX_GPPI_V1)
  zephyr_library_sources_ifdef(CONFIG_SOC_NRF54H20_CPURAD nrfx_gppi_cpurad.c)
  zephyr_library_sources_ifdef(CONFIG_SOC_NRF54H20_CPUAPP nrfx_gppi_nrf54h_global.c)
endif()

zephyr_include_directories(.)

# Ensure that image size aligns with 16 bytes so that MRAMC finalizes all writes
# for the image correctly
zephyr_linker_sources(SECTIONS SORT_KEY zzz_place_align_at_end align.ld)

add_subdirectory(bicr)
