# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(cs47l63_test)

target_sources(app PRIVATE
    src/cs47l63_emul.c
    src/test_clock_solver.c
    src/test_clock_apply.c
    src/test_boot.c
    src/test_dai_configure.c
    src/test_output_path.c
    src/test_input_path.c
    src/test_fault.c
)

target_include_directories(app PRIVATE src ${ZEPHYR_BASE}/drivers/audio)
