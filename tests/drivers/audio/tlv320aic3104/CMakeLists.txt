# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(tlv320aic3104_test)

target_sources(app PRIVATE
    src/tlv320aic3104_emul.c
    src/test_clock_solver.c
    src/test_clock_apply.c
    src/test_dai_configure.c
    src/test_channel_mode.c
    src/test_route_output.c
    src/test_route_input.c
    src/test_fault.c
    src/test_independent_power.c
    src/test_init_properties.c
    src/test_pga_gain.c
    src/test_read_path.c
    src/test_volume_restore.c
    src/test_configure_fail.c
)

target_include_directories(app PRIVATE src ${ZEPHYR_BASE}/drivers/audio)
