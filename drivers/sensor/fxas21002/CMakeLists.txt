# Makefile - FXAS21002 3-axis gyroscope
#
# Copyright (c) 2017, NXP
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            IFDEF:${CONFIG_FXAS21002}         fxas21002.c
            IFDEF:${CONFIG_FXAS21002_TRIGGER} fxas21002_trigger.c
)

target_sources(drivers PRIVATE ${PRIVATE_SOURCES})
