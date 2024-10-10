# Copyright (c) 2024 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(arm_v7m_nvic.c)
zephyr_linker_sources(ROM_START SORT_KEY 0x0vectors irq-vector-table.ld)
