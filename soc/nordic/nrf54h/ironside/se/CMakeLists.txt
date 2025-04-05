# Copyright (c) 2025 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources_ifdef(CONFIG_SOC_NRF54H20_IRONSIDE_SE_BOOT_REPORT ironside_se_boot_report.c)
zephyr_include_directories(include)
