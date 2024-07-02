# Copyright (c) 2024 Brill Power Ltd.
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_CRC_STM32 crc_stm32.c)
