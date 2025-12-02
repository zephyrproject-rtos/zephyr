# Copyright 2022 NXP
# SPDX-License-Identifier: Apache-2.0

add_subdirectory(${SOC_SERIES})
add_subdirectory(common)

zephyr_include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../common)
