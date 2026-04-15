# SPDX-FileCopyrightText: Copyright 2024
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
set(TFLM_BUILD_FROM_SOURCE TRUE)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(tflm_neutron)

# Add application sources (exclude model.hpp from glob)
file(GLOB app_sources src/*.c src/*.cpp src/*.h)
target_sources(app PRIVATE ${app_sources})

# Neutron NPU custom op for TFLM
if(CONFIG_TENSORFLOW_LITE_MICRO_NXP_NEUTRON)
    target_sources(app PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm/neutron.cpp
    )
    target_include_directories(app PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/tflm
    )
endif()

# Board-specific model include path
if(CONFIG_BOARD_MIMXRT700_EVK OR CONFIG_SOC_MIMXRT798S)
    target_sources(app PRIVATE boards/mimxrt700_evk.c)
    target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/models/rt700)
elseif(CONFIG_BOARD_FRDM_MCXN947 OR CONFIG_SOC_MCXN947)
    target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/models/mcxn)
endif()
