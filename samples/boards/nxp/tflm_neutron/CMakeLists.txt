# SPDX-FileCopyrightText: Copyright 2026
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(tflm_neutron)

set(NEUTRON_BLOB_PATH "${ZEPHYR_HAL_NXP_MODULE_DIR}/zephyr/blobs/neutron/rt700/cm33/libNeutronDriver.a")

if(NOT EXISTS "${NEUTRON_BLOB_PATH}")
    message(STATUS "------------------------------------------------------------")
    message(STATUS " [AUTOMATION] Missing NXP Neutron NPU Binary Blob!")
    message(STATUS " [AUTOMATION] Trying 'west blobs fetch hal_nxp' automatically...")
    message(STATUS "------------------------------------------------------------")

    execute_process(
        COMMAND west blobs fetch hal_nxp --auto-accept
        WORKING_DIRECTORY ${WEST_TOPDIR}
        RESULT_VARIABLE WEST_FETCH_RESULT
    )
endif()

# Add application sources (exclude model.hpp from glob)
file(GLOB app_sources src/*.c src/*.cpp src/*.h)
target_sources(app PRIVATE ${app_sources})

# Board-specific model include path
if(CONFIG_SOC_MIMXRT798S)
    target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/models/rt700)
elseif(CONFIG_SOC_MCXN947)
    target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/models/mcxn)
endif()
