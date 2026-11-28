# Copyright (c) 2026 StructCore Embedded
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(snmp_agent_sample)

target_sources(app PRIVATE
    src/main.c
    src/app_led.c
    src/app_sensor.c
    src/app_button.c
    src/app_mib.c
)
