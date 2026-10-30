# SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(host_ecc_context)

target_include_directories(app PRIVATE ${ZEPHYR_BASE}/subsys/bluetooth)
target_sources(app PRIVATE src/main.c)

# Holds or fails the key generations, see __wrap_psa_generate_key()
target_link_options(app PUBLIC -Wl,--wrap=psa_generate_key)
