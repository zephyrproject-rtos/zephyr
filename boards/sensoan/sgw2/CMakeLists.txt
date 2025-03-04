# Kconfig - SGW-2 board configuration
#
# Copyright (c) 2019 Nordic Semiconductor ASA
# Copyright (c) 2024 Sensoan Oy
#
# SPDX-License-Identifier: Apache-2.0

if((CONFIG_BOARD_SGW2_NRF9160 OR CONFIG_BOARD_SGW2_NRF9160_NS) AND CONFIG_BT_HCI)
  zephyr_library()
  zephyr_library_sources(nrf5340_reset.c)
endif()
