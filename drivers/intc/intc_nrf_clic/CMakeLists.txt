# Copyright (c) 2025 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(nrf_clic.c)
zephyr_library_sources(nrf_clic.S)
zephyr_linker_sources(ROM_START SORT_KEY 0x0vectors irq-vector-table.ld)
