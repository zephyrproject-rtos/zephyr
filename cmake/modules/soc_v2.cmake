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
  # This populates SOC_V2_DIR based on the HWM type in use by the SOC.
  string(TOUPPER SOC_FAMILY_${SOC_FAMILY}_DIR family_setting_dir)
  string(TOUPPER SOC_SERIES_${SOC_SERIES}_DIR series_setting_dir)
  string(TOUPPER SOC_${SOC_NAME}_DIR soc_setting_dir)

  if(DEFINED ${soc_setting_dir})
    set(SOC_V2_DIR ${${soc_setting_dir}})
  elseif(DEFINED ${series_setting_dir})
    set(SOC_V2_DIR ${${series_setting_dir}})
  elseif(DEFINED ${family_setting_dir})
    set(SOC_V2_DIR ${${family_setting_dir}})
  else()
    message(FATAL_ERROR "No soc directory found for SoC: ${SOC_NAME}, "
                        "series: ${SOC_SERIES}, family: ${SOC_FAMILY}")
  endif()
  set(SOC_FULL_DIR ${SOC_V2_DIR})
endif()
