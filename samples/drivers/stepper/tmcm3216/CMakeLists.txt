# SPDX-FileCopyrightText: Copyright (c) 2025 Alexis Czezar C Torreno
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(stepper_tmcm3216)

target_sources(app PRIVATE src/main.c)
