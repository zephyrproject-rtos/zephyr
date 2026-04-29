# Texas Instruments AM26x MCU Family
#
# Copyright (c) 2026 Texas Instruments Incorporated
#
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_SOC_SERIES_AM261X_R5F)
  zephyr_sources(r5/soc.c)

  zephyr_include_directories(r5)
  zephyr_include_directories(common)

  zephyr_sources(common/ctrl_partitions.c)

  zephyr_sources_ifdef(CONFIG_ARM_MPU mpu_regions.c)
endif()
