# Makefile - FXAS21002 3-axis gyroscope
#
# Copyright (c) 2017, NXP
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_sources_ifdef(CONFIG_FXAS21002 fxas21002.c)
zephyr_sources_ifdef(CONFIG_FXAS21002_TRIGGER fxas21002_trigger.c)
