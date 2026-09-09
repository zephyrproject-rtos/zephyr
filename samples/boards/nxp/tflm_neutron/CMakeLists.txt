# SPDX-FileCopyrightText: Copyright 2026
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(tflm_neutron)

# Add application sources (exclude model.hpp from glob)
file(GLOB app_sources src/*.c src/*.cpp src/*.h)
target_sources(app PRIVATE ${app_sources})

# Board-specific model include path and Neutron blob location
if(CONFIG_SOC_MIMXRT798S)
  target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/models/rt700)
  set(neutron_blob_dir "${ZEPHYR_HAL_NXP_MODULE_DIR}/zephyr/blobs/neutron/rt700/cm33")
elseif(CONFIG_SOC_MCXN947)
  target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/models/mcxn)
  set(neutron_blob_dir "${ZEPHYR_HAL_NXP_MODULE_DIR}/zephyr/blobs/neutron/mcxn")
endif()

# The Neutron NPU driver and firmware are distributed as binary blobs. Report
# their absence here instead of letting the link step fail on a missing
# archive.
if(NOT EXISTS "${neutron_blob_dir}/libNeutronDriver.a")
  message(FATAL_ERROR
        "The NXP Neutron NPU binary blobs required by this sample are not "
        "present. Fetch them with:\n"
        "  west blobs fetch hal_nxp")
endif()
