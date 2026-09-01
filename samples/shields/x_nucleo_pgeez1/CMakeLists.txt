# SPDX-FileCopyrightText: Copyright 2026 EXALT Technologies
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)

# This sample requires the M95P32 provided by the X-NUCLEO-PGEEZ1 shield.
set(SHIELD x_nucleo_pgeez1)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(x_nucleo_pgeez1)

target_sources(app PRIVATE src/main.c)
