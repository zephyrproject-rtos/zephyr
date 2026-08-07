# Copyright (c) 2024 Renesas Electronics Corporation
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(include)
zephyr_include_directories(common)

add_subdirectory(common)
add_subdirectory(${SOC_SERIES})
