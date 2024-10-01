# Copyright (c) 2024 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_ARM)
  zephyr_library_sources(soc.c)
  if(CONFIG_PM OR CONFIG_POWEROFF)
    zephyr_library_sources(power.c)
  endif()
endif()

zephyr_library_sources_ifdef(CONFIG_PM_S2RAM pm_s2ram.c)

zephyr_include_directories(.)

# Ensure that image size aligns with 16 bytes so that MRAMC finalizes all writes
# for the image correctly
zephyr_linker_sources(SECTIONS SORT_KEY zzz_place_align_at_end align.ld)
