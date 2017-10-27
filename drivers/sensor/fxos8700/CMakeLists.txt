# Makefile - FXOS8700 6-axis accelerometer/magnetometer
#
# Copyright (c) 2016, Freescale Semiconductor, Inc.
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_sources_ifdef(CONFIG_FXOS8700 fxos8700.c)
zephyr_sources_ifdef(CONFIG_FXOS8700_TRIGGER fxos8700_trigger.c)
