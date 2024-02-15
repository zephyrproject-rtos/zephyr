# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) 2021, Nordic Semiconductor ASA

# Configure SoC settings based on Kconfig settings.
#
# This CMake module will set the following variables in the build system based
# on Kconfig settings for the selected SoC.
#
# Outcome:
# The following variables will be defined when this CMake module completes:
#
# - SOC_NAME:   Name of the SoC in use, identical to CONFIG_SOC
# - SOC_SERIES: Name of the SoC series in use, identical to CONFIG_SOC_SERIES
# - SOC_FAMILY: Name of the SoC family, identical to CONFIG_SOC_FAMILY
#
# Variables set by this module and not mentioned above are considered internal
# use only and may be removed, renamed, or re-purposed without prior notice.

include_guard(GLOBAL)

include(kconfig)

if(HWMv2)
  set(SOC_NAME   ${CONFIG_SOC})
  set(SOC_SERIES ${CONFIG_SOC_SERIES})
  set(SOC_TOOLCHAIN_NAME ${CONFIG_SOC_TOOLCHAIN_NAME})
  set(SOC_FAMILY ${CONFIG_SOC_FAMILY})
  set(SOC_V2_DIR ${SOC_${SOC_NAME}_DIR})
  set(SOC_FULL_DIR ${SOC_V2_DIR})
endif()
