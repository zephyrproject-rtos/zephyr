#
# Copyright (c) 2021 Linaro Limited
#
# SPDX-License-Identifier: Apache-2.0
#

# Building TFM requires Cmake 3.15.
cmake_minimum_required(VERSION 3.15)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(psa_custom_service)

target_sources(app PRIVATE src/main.c)
target_sources(app PRIVATE src/util_app_log.c)
target_sources(app PRIVATE src/util_sformat.c)

target_include_directories(app PRIVATE
  ${ZEPHYR_TRUSTED_FIRMWARE_M_MODULE_DIR}/trusted-firmware-m/interface/include
)

# In TF-M, for release build, all the logs are silenced. At least enable
# `LOG_LEVEL_INFO`
set_property(TARGET zephyr_property_target
             APPEND PROPERTY TFM_CMAKE_OPTIONS
             -DTFM_SPM_LOG_LEVEL=TFM_SPM_LOG_LEVEL_INFO
             -DTFM_PARTITION_LOG_LEVEL=TFM_PARTITION_LOG_LEVEL_INFO
)
