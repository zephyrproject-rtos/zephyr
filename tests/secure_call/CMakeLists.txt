# SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
# SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(secure_call_test)

# CONFIG_APPLICATION_DEFINED_SYSCALL (set in prj.conf) causes the root
# CMakeLists.txt to add APPLICATION_SOURCE_DIR to parse_syscalls_include_args.
# Our parse_secure_calls.py custom command reuses that list, so test_calls.h
# is scanned automatically and <zephyr/secure_calls/test_calls.h> is generated
# alongside the normal Zephyr-tree secure call headers.

target_sources(app PRIVATE src/main.c)
