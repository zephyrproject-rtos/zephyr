# Copyright (c) 2024 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

zephyr_library_sources(soc_idle.c soc_irq.S soc_irq.c vector.S)
