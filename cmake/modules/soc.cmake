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
# - SOC_FULL_DIR: Full directory of where the SoC files are located
# - SOC_DIRECTORIES: List of directories where SoC files which include this SoC are located
# - SOC_TOOLCHAIN_NAME: Optional toolchain name of the SoC
#
# Variables set by this module and not mentioned above are considered internal
# use only and may be removed, renamed, or re-purposed without prior notice.

include_guard(GLOBAL)

include(kconfig)

function(deprecated_soc_var variable access value current_list_file stack)
  message(DEPRECATION "Variable ${variable} is deprecated, please check the Zephyr 4.5 migration "
    "guide and update usage of this variable."
  )
endfunction()

set(SOC_TOOLCHAIN_NAME ${CONFIG_SOC_TOOLCHAIN_NAME})
set(SOC_FULL_DIR ${SOC_${CONFIG_SOC}_DIR} CACHE PATH "Path to the SoC directory." FORCE)
set(SOC_DIRECTORIES ${SOC_${CONFIG_SOC}_DIRECTORIES} CACHE INTERNAL
    "List of SoC directories for SoC (${CONFIG_SOC})" FORCE
)

find_package(Deprecated COMPONENTS soc_vars)

foreach(dir ${SOC_DIRECTORIES})
  set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${dir}/soc.yml)
endforeach()
