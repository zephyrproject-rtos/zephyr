# Copyright 2022 Meta
# SPDX-License-Identifier: Apache-2.0

project(time_units)
set(SOURCES main.c overflow.c)
find_package(ZephyrUnittest REQUIRED HINTS $ENV{ZEPHYR_BASE})
