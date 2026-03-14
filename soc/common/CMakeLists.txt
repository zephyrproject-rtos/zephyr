# Copyright (c) 2024 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(CONFIG_SOC_CPU_CLOCK_PUBLISH cpu_clock.c)

add_subdirectory_ifdef(CONFIG_RISCV_PRIVILEGED riscv-privileged)
