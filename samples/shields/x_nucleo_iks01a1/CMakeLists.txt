# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

# This sample is specific to x_nucleo_iks01a1 shield. Enforce -DSHIELD option
set(SHIELD x_nucleo_iks01a1)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(x_nucleo_iks01a1)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})
