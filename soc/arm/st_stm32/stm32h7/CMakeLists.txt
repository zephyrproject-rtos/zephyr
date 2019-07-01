# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(${ZEPHYR_BASE}/drivers)

zephyr_sources_ifdef(CONFIG_CPU_CORTEX_M7 soc_m7.c)
