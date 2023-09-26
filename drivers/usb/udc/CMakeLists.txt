# Copyright (c) 2021 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(udc_common.c)
zephyr_library_sources_ifdef(CONFIG_UDC_NRF udc_nrf.c)
zephyr_library_sources_ifdef(CONFIG_UDC_KINETIS udc_kinetis.c)
zephyr_library_sources_ifdef(CONFIG_UDC_SKELETON udc_skeleton.c)
zephyr_library_sources_ifdef(CONFIG_UDC_VIRTUAL udc_virtual.c)
zephyr_library_sources_ifdef(CONFIG_UDC_STM32 udc_stm32.c)
