# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) 2021, Nordic Semiconductor ASA

#[=======================================================================[.rst:
soc
***

Configure SoC settings based on Kconfig settings.

This module sets up SoC-specific variables in the build system based on Kconfig
settings for the selected SoC. It handles both legacy and HWMv2 SoC configurations.

Variables
=========

The following variables will be defined when this CMake module completes:

* :cmake:variable:`SOC_NAME`
* :cmake:variable:`SOC_SERIES`
* :cmake:variable:`SOC_FAMILY`

Variables set by this module and not mentioned above are considered internal use only and may be
removed, renamed, or re-purposed without prior notice.

Usage
=====

This module is typically loaded automatically by the Zephyr build system based on the selected SoC.
It sets up the necessary variables for SoC-specific configuration and build settings.

Example
-------

.. code-block:: cmake

   # The module is loaded automatically when building for a specific SoC
   # The following variables will be available:
   message(STATUS "Building for SoC: ${SOC_NAME}")
   message(STATUS "SoC series: ${SOC_SERIES}")
   message(STATUS "SoC family: ${SOC_FAMILY}")
#]=======================================================================]

include_guard(GLOBAL)

include(kconfig)

set(SOC_NAME   ${CONFIG_SOC})
set(SOC_SERIES ${CONFIG_SOC_SERIES})
set(SOC_TOOLCHAIN_NAME ${CONFIG_SOC_TOOLCHAIN_NAME})
set(SOC_FAMILY ${CONFIG_SOC_FAMILY})
set(SOC_V2_DIR ${SOC_${SOC_NAME}_DIR})
set(SOC_FULL_DIR ${SOC_V2_DIR} CACHE PATH "Path to the SoC directory." FORCE)
set(SOC_DIRECTORIES ${SOC_${SOC_NAME}_DIRECTORIES} CACHE INTERNAL
    "List of SoC directories for SoC (${SOC_NAME})" FORCE
)
foreach(dir ${SOC_DIRECTORIES})
  set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${dir}/soc.yml)
endforeach()
