# Copyright (c) 2024 Bjarki Arge Andreasen
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(test_sys_irq_direct_irq)

target_sources(app PRIVATE src/test.c)
