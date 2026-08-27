# Copyright (c) 2025 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources(soc.c)

if(CONFIG_BUILD_WITH_TFM)
  set_property(TARGET zephyr_property_target APPEND PROPERTY TFM_CMAKE_OPTIONS
    -DZEPHYR_BASE=${ZEPHYR_BASE}
  )
endif()

# WICR programming requires MRAMC (secure-only peripheral).
# For TF-M builds, wicr_setup.c is compiled into the secure image
# from modules/trusted-firmware-m/nordic/nrf7120_cpuapp/CMakeLists.txt.
if(CONFIG_SOC_NRF7120_WICR_SETUP AND NOT CONFIG_BUILD_WITH_TFM)
  zephyr_library_sources(wicr_setup.c)
endif()

zephyr_include_directories(.)

# Ensure that image size aligns with 16 bytes so that MRAMC finalizes all writes
# for the image correctly
zephyr_linker_sources(SECTIONS SORT_KEY zzz_place_align_at_end align.ld)
