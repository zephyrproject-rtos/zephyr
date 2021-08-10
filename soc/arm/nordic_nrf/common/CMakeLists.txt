# Copyright (c) 2021 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources_ifdef(CONFIG_SOC_FAMILY_NRF soc_nrf_common.S)
zephyr_include_directories(.)
