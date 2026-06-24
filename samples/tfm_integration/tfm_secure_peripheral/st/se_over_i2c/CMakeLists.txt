# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

get_target_property(TFM_BINARY_DIR tfm TFM_BINARY_DIR)

configure_file(
  ${CMAKE_CURRENT_LIST_DIR}/se_over_i2c/tfm_manifest_list.yaml.in
  ${CMAKE_CURRENT_BINARY_DIR}/se_over_i2c/tfm_manifest_list.yaml
)

if(CONFIG_TFM_IPC)
    set(SPM_BACKEND "IPC")
else()
    set(SPM_BACKEND "SFN")
endif()

configure_file(
  ${CMAKE_CURRENT_LIST_DIR}/se_over_i2c/tfm_se_over_i2c.yaml.in
  ${CMAKE_CURRENT_BINARY_DIR}/se_over_i2c/tfm_se_over_i2c.yaml
)

set_property(TARGET zephyr_property_target
  APPEND PROPERTY TFM_CMAKE_OPTIONS
  -DTFM_EXTRA_MANIFEST_LIST_FILES=${CMAKE_CURRENT_BINARY_DIR}/se_over_i2c/tfm_manifest_list.yaml
  -DTFM_EXTRA_PARTITION_PATHS=${CMAKE_CURRENT_LIST_DIR}/se_over_i2c
)

project(tfm_se_over_i2c)

target_sources(app PRIVATE
  src/main.c
  src/se_over_i2c.c
)

target_include_directories(app PRIVATE
  $<TARGET_PROPERTY:tfm,TFM_BINARY_DIR>/api_ns/interface/include
)
