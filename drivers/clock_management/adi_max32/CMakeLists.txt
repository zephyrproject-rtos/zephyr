# Copyright 2026 Analog Devices, Inc.
# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources_ifdef(CONFIG_CLOCK_MANAGEMENT_ADI_MAX32_SOURCE max32_source.c)
zephyr_library_sources_ifdef(CONFIG_CLOCK_MANAGEMENT_ADI_MAX32_DIV max32_div.c)

# Header
add_clock_management_header_ifdef(CONFIG_CLOCK_MANAGEMENT_ADI_MAX32 max32_clocks.h)
