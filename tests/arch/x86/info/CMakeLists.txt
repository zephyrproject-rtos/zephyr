# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2019 Intel Corp.

cmake_minimum_required(VERSION 3.13.1)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(x86_info)

target_sources(app PRIVATE src/main.c)
target_sources(app PRIVATE src/timer.c)
target_sources(app PRIVATE src/memmap.c)
target_sources_ifdef(CONFIG_ACPI app PRIVATE src/acpi.c)
target_sources_ifdef(CONFIG_MULTIBOOT app PRIVATE src/multiboot.c)
