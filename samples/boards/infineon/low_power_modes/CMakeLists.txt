# SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
# SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(low_power_modes)

target_sources(app PRIVATE src/main.c)
target_sources(app PRIVATE src/main_dut.c)
target_sources(app PRIVATE src/peripherals.c)
