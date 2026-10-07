# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

# This sample is specific to the adi_lsmspg shield.
set(SHIELD adi_lsmspg)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(adi_lsmspg_curve_tracer)

target_sources(app PRIVATE src/main.c)
