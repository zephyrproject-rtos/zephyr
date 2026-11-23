# SPDX-FileCopyrightText: Copyright 2026 NXP
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

# The sample stores its wake counter in the iRTC's unused alarm registers, which
# survive deep power down. Pull in the MCUX iRTC driver for its write-protection
# unlock helper.
set(CONFIG_MCUX_COMPONENT_driver.irtc ON)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(mimxrt700_evk_system_off)

target_sources(app PRIVATE src/main.c)
