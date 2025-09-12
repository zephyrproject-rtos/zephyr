# Copyright 2024 NXP
# SPDX-License-Identifier: Apache-2.0

add_subdirectory(${SOC_SERIES})

zephyr_include_directories(.)
zephyr_include_directories(${SOC_SERIES})

zephyr_include_directories(${SOC_SERIES}/include)

zephyr_include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../common)
