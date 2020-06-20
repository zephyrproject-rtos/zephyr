# Copyright (c) 2020 STMicroelectronics
#
# SPDX-License-Identifier: Apache-2.0
#
cmake_minimum_required(VERSION 3.13.1)

# This sample is specific to x_nucleo_iks02a1 shield. Enforce -DSHIELD option
set(SHIELD x_nucleo_iks02a1_shub)

# IKS02A1 board configured in sensorhub mode
set(CONF_FILE shub.conf)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(x_nucleo_iks02a1)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})
