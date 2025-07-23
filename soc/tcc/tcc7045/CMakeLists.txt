# Copyright 2024 Hounjoung Rim <hounjoung@tsnlab.com>
#
# SPDX-License-Identifier: Apache-2.0

zephyr_sources(
	soc.c
)

zephyr_sources_ifdef(
  CONFIG_ARM_MPU
  arm_mpu_regions.c
)

zephyr_include_directories(.)

if(CONFIG_SOC_TCC7045)
  set(SOC_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld CACHE INTERNAL "")
endif()
