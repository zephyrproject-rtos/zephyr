# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(arm_mpu_pxn)

target_sources(app PRIVATE src/main.c)
zephyr_linker_sources_ifdef(CONFIG_SOC_MPS3_CORSTONE300 RAM_SECTIONS mps3_corstone300.ld)
zephyr_linker_sources_ifdef(CONFIG_SOC_MPS3_CORSTONE310 RAM_SECTIONS mps3_corstone310.ld)
