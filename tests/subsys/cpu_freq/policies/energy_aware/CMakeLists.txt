# Copyright 2026 NXP
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(cpu_freq_energy_aware_test)

target_sources(app PRIVATE src/main.c)

zephyr_ld_options(
	-Wl,--wrap=cpu_workload_estimate_get
	-Wl,--wrap=cpu_freq_pstate_current_get
)
