# Copyright (c) 2019 Nordic Semiconductor ASA.
# SPDX-License-Identifier: Apache-2.0

if (CONFIG_BOARD_NRF5340_DK_NRF5340_CPUAPP)
zephyr_library()
zephyr_library_sources(nrf5340_cpunet_reset.c)
endif()
