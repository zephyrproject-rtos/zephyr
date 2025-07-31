# Copyright (c) 2024 VCI Development - LPC54S018J4MET180E
# Private Porting , by David Hor - Xtooltech 2025, david.hor@xtooltech.com
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(can_sample)

target_sources(app PRIVATE main.c)