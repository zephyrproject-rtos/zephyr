# Makefile - MAX30101 heart rate sensor
#
# Copyright (c) 2017, NXP
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            IFDEF:${CONFIG_MAX30101} max30101.c
)

target_sources(drivers PRIVATE ${PRIVATE_SOURCES})
