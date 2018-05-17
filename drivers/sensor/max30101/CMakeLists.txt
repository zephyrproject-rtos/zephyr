# Makefile - MAX30101 heart rate sensor
#
# Copyright (c) 2017, NXP
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_sources_ifdef(CONFIG_MAX30101 max30101.c)
