#
# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_MOCKUP_BATTERY_SENSOR battery.c)
