#
# Copyright 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

cmake_minimum_required(VERSION 3.20.0)

if(CONFIG_MP)
    zephyr_library_named(mp)

    add_subdirectory(core)
    add_subdirectory(plugins)
endif()
