# SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
# SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

# In a sysbuild run the S image builds in a sibling directory named "secure_s".
# Point tz/CMakeLists.txt at that directory so it can find libentryveneers.a.
cmake_path(GET CMAKE_CURRENT_BINARY_DIR PARENT_PATH _sb_root)
set(ARM_VENEERS_LIB_DIR "${_sb_root}/secure_s" CACHE PATH
    "Directory containing libentryveneers.a from the Secure image build" FORCE)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(secure_call_tz_ns)

target_sources(app PRIVATE src/main.c)
