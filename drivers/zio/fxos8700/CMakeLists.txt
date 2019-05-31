# Makefile - FXOS8700 6-axis accelerometer/magnetometer
#
# Copyright (c) 2016, Freescale Semiconductor, Inc.
# Copyright (c) 2019, Thomas Burdick <thomas.burdick@gmail.com>
#
# SPDX-License-Identifier: Apache-2.0
#
zephyr_library()

zephyr_library_sources_ifdef(CONFIG_ZIO_FXOS8700 fxos8700.c)
