# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

# Apply common TI SimpleLink config for all lp_em_cc* boards.
# Must be set before find_package(Zephyr) so Kconfig picks it up.
if("${BOARD}" MATCHES "^lp_em_cc")
    list(APPEND OVERLAY_CONFIG
        "${CMAKE_CURRENT_SOURCE_DIR}/boards/ti_simplelink.conf")
endif()

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(mesh_shell)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})
