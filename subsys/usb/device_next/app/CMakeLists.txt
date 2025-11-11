# Copyright (c) 2024 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(CONFIG_CDC_ACM_SERIAL_INITIALIZE_AT_BOOT cdc_acm_serial.c)
