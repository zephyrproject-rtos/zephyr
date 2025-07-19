# Copyright (c) 2024 Realtek Semiconductor Corp.
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(common)

add_subdirectory(common)
add_subdirectory(${CONFIG_SOC})
