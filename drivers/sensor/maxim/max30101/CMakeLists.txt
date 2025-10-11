# Makefile - MAX30101 heart rate sensor
#
# Copyright (c) 2017, NXP
# Copyright (c) 2025, CATIE
#
# SPDX-License-Identifier: Apache-2.0
#
zephyr_library()

zephyr_library_sources(max30101.c)
zephyr_library_sources_ifdef(CONFIG_MAX30101_TRIGGER max30101_trigger.c)
