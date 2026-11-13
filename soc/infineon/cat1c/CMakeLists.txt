# Copyright (c) 2024 Cypress Semiconductor Corporation.
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(${ZEPHYR_BASE}/soc/infineon/common)

add_subdirectory(common)
add_subdirectory(${CONFIG_SOC_SERIES})
