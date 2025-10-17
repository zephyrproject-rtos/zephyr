# Copyright (c) 2025 Silicon Laboratories Inc.
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

# zephyr-keep-sorted-start
zephyr_library_sources_ifdef(CONFIG_DEBUG_CORESIGHT_NRF debug_coresight_nrf.c)
zephyr_library_sources_ifdef(CONFIG_DEBUG_NRF_ETR debug_nrf_etr.c)
zephyr_library_sources_ifdef(CONFIG_DEBUG_SILABS_PTI debug_silabs_pti.c)
# zephyr-keep-sorted-stop
