# Copyright (c) 2025 Alif Semiconductor
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)
zephyr_include_directories(../common)

# Cluster-specific code for Cortex-M55 RTSS cores
if(CONFIG_SOC_FAMILY_ENSEMBLE_RTSS)
  add_subdirectory(rtss)
endif()

# Cluster-specific code for Cortex-A32 APSS cores
if(CONFIG_SOC_FAMILY_ENSEMBLE_APSS)
  add_subdirectory(apss)
endif()

add_subdirectory(${SOC_SERIES})
