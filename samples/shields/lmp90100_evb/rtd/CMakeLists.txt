# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)

# This sample is specific to lmp90100_evb shield. Enforce -DSHIELD option
set(SHIELD lmp90100_evb)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(lmp90100_evb_rtd)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})
