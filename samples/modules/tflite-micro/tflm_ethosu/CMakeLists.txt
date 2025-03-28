# Copyright 2021-2022 Arm Limited and/or its affiliates <open-source-office@arm.com>
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(tflm_ethosu_app)

# Choose appropriate model
if(CONFIG_TAINT_BLOBS_TFLM_ETHOSU)
  # Vela-compiled model must match Ethos-U RTL config, e.g. MACs per cycle
  target_include_directories_ifdef(CONFIG_BOARD_MPS3 app PRIVATE src/models/keyword_spotting_cnn_small_int8_ethos_u55_128)
  target_include_directories_ifdef(CONFIG_BOARD_NUMAKER_M55M1 app PRIVATE src/models/keyword_spotting_cnn_small_int8_ethos_u55_256)
elseif(CONFIG_TAINT_BLOBS_TFLM)
  target_include_directories(app PRIVATE src/models/keyword_spotting_cnn_small_int8)
endif()

target_sources(app PRIVATE src/main.cpp src/inference_process.cpp)
target_sources_ifdef(CONFIG_BOARD_NUMAKER_M55M1 app PRIVATE src/npu_cache.c)

# Add platform specific linker source snippet
zephyr_linker_sources_ifdef(CONFIG_BOARD_MPS3 SECTIONS linker.ld)
