#
# Copyright 2024 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_sources(soc.c)

if(CONFIG_SOC_MIMXRT1189_CM33)
  zephyr_sources(m33/mpu_regions.c)
  zephyr_linker_sources(DTCM_SECTION m33/dtcm.ld)
endif()

zephyr_include_directories(.)

set(SOC_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld CACHE INTERNAL "")
