# Copyright (c) 2024, Nordic Semiconductor ASA
# Copyright (c) 2025 Analog Devices, Inc.
#
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources(cpu_load.c)
zephyr_library_sources_ifdef(CONFIG_CPU_LOAD_BACKEND_IDLE_HOOK cpu_load_backend_idle.c)
zephyr_library_sources_ifdef(CONFIG_CPU_LOAD_BACKEND_RUNTIME_STATS cpu_load_backend_runtime.c)
