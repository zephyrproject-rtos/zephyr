# Copyright (c) 2024 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources_ifdef(CONFIG_SOC_NRF54H20_GPD gpd.c)
zephyr_include_directories(include)
