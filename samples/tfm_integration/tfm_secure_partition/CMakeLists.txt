#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#

cmake_minimum_required(VERSION 3.13.1)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

configure_file(
  ${CMAKE_CURRENT_LIST_DIR}/dummy_partition/tfm_manifest_list.yaml
  ${CMAKE_CURRENT_BINARY_DIR}/dummy_partition/tfm_manifest_list.yaml)

set_property(TARGET zephyr_property_target
  APPEND PROPERTY TFM_CMAKE_OPTIONS
  -DTFM_EXTRA_MANIFEST_LIST_PATH=${CMAKE_CURRENT_BINARY_DIR}/dummy_partition/tfm_manifest_list.yaml
  -DTFM_EXTRA_PARTITION_CMAKE_PATH=${CMAKE_CURRENT_LIST_DIR}/dummy_partition
  )

project(tfm_secure_partition)

target_sources(app PRIVATE src/main.c)

target_include_directories(app PRIVATE
  ${ZEPHYR_TRUSTED_FIRMWARE_M_MODULE_DIR}/trusted-firmware-m/interface/include
)

target_compile_definitions(app
    PRIVATE TFM_PARTITION_DUMMY_PARTITION)
