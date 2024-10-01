# Makefile - FXLS8974 3-axis accelerometer
#
# Copyright 2024 NXP
#
# SPDX-License-Identifier: Apache-2.0
#
zephyr_library()

zephyr_library_sources(fxls8974.c)
zephyr_library_sources_ifdef(CONFIG_FXLS8974_TRIGGER fxls8974_trigger.c)
