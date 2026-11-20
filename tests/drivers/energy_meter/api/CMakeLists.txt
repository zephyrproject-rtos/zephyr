# Copyright (c) 2025 Pedro Ramos
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(energy_meter_api)

target_sources(app PRIVATE src/fake_meter.c src/test_api.c src/test_shell.c)
