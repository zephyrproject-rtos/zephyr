# Copyright (c) 2025 Microchip Technology Inc.
# SPDX-License-Identifier: Apache-2.0

add_subdirectory(common)
add_subdirectory_ifdef(CONFIG_SOC_SERIES_PIC32CZ_CA70 pic32cz_ca70)

zephyr_include_directories(${CONFIG_SOC_SERIES})
