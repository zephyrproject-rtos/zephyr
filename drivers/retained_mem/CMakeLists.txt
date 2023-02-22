# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_RETAINED_MEM_NRF_GPREGRET retained_mem_nrf_gpregret.c)
