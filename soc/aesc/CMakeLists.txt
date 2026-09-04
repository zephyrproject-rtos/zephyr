# Copyright (c) 2025 Aesc Silicon
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

add_subdirectory(${CONFIG_SOC_SERIES})
add_subdirectory(common)
