# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2026 Fabien Proriol

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(knx_stack_loopback)

# Unlike the other knx suites, nothing here #includes a stack .c file: the
# subsystem and the driver are built exactly as an application gets them
# (subsys/knx/CMakeLists.txt + drivers/knx/CMakeLists.txt), so knx_core.c,
# layer1_physical.c and the L1 driver are compiled as configured.
target_sources(app PRIVATE src/main.c)
