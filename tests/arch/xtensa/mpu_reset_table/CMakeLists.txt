# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(mpu_reset_table)

target_sources(app PRIVATE src/main.c)

# The MPU driver replaces the map in z_prep_c(); read it just before that.
zephyr_ld_options(-Wl,--wrap=xtensa_mpu_init)
