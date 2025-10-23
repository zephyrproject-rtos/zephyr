# Copyright 2021-2022 Arm Limited and/or its affiliates <open-source-office@arm.com>
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(tflm_ethosu_app)

# Choose model
target_include_directories(app PRIVATE src/models/keyword_spotting_cnn_small_int8)

target_sources(app PRIVATE src/main.cpp src/inference_process.cpp)

# Add platform specific linker source snippet
zephyr_linker_sources_ifdef(CONFIG_SOC_SERIES_MPS3 SECTIONS linker.ld)
zephyr_linker_sources_ifdef(CONFIG_SOC_SERIES_MPS4 SECTIONS linker.ld)
