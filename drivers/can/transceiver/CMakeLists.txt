# Copyright (c) 2022 Vestas Wind Systems A/S
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_property(ALLOW_EMPTY TRUE)
zephyr_library_sources_ifdef(CONFIG_CAN_TRANSCEIVER_GPIO can_transceiver_gpio.c)
