# Copyright (c) 2025 James Bennion-Pedley
# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources_ifdef(CONFIG_PM power.c)
zephyr_library_sources_ifdef(CONFIG_ARCH_HAS_CUSTOM_CPU_IDLE soc_idle.c)

zephyr_include_directories(.)
