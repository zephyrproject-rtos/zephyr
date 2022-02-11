#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#

cmake_minimum_required(VERSION 3.20)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

# TFM_BINARY_DIR is used by configure_file
get_target_property(TFM_BINARY_DIR tfm TFM_BINARY_DIR)

configure_file(
  ${CMAKE_CURRENT_LIST_DIR}/dummy_partition/tfm_manifest_list.yaml.in
  ${CMAKE_CURRENT_BINARY_DIR}/dummy_partition/tfm_manifest_list.yaml
)

set_property(TARGET zephyr_property_target
  APPEND PROPERTY TFM_CMAKE_OPTIONS
  -DTFM_EXTRA_MANIFEST_LIST_FILES=${CMAKE_CURRENT_BINARY_DIR}/dummy_partition/tfm_manifest_list.yaml
  -DTFM_EXTRA_PARTITION_PATHS=${CMAKE_CURRENT_LIST_DIR}/dummy_partition
)

project(tfm_secure_partition)

zephyr_library_sources(
  src/main.c
  src/dummy_partition.c
)

zephyr_library_link_libraries(tfm_api)

zephyr_library_compile_definitions(
  TFM_PARTITION_DUMMY_PARTITION
)
