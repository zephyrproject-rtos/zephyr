# Copyright 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(frdm_mcxa156_lpadc_opamp_lpdac)

target_sources(app PRIVATE src/main.c)
