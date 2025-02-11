# Makefile - FXOS8700 6-axis accelerometer/magnetometer
#
# Copyright (c) 2016, Freescale Semiconductor, Inc.
#
# SPDX-License-Identifier: Apache-2.0
#
zephyr_library()

zephyr_library_sources(fxos8700.c)
zephyr_library_sources_ifdef(CONFIG_FXOS8700_TRIGGER fxos8700_trigger.c)
