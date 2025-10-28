#/*
# * Copyright 2025 NXP
# *
# * SPDX-License-Identifier: Apache-2.0
# */

cmake_minimum_required(VERSION 3.20.0)

if (CONFIG_MP)
    set(MP_BASE_DIR ${CMAKE_CURRENT_LIST_DIR})
    set(MP_CORE_DIR ${MP_BASE_DIR}/src/core)
    set(MP_PLUGIN_DIR ${MP_BASE_DIR}/src/plugins)

    message("Building MediaPipe library")

    find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

    zephyr_library_named(mp)

    zephyr_include_directories(
        ${MP_BASE_DIR}
    )

    file(GLOB MP_CORE_SOURCE_FILES
        ${MP_CORE_DIR}/*.c
        ${MP_CORE_DIR}/utils/*.c
    )

    zephyr_library_sources(
        ${MP_BASE_DIR}/mp.c
        ${MP_CORE_SOURCE_FILES}
    )

    # Plugins
    if (CONFIG_MP_PLUGIN_ZVID)
        message("Including zvid plugin")
        file(GLOB_RECURSE PLUGIN_ZVID_SOURCE_FILES ${MP_PLUGIN_DIR}/zvid/*.c)
        zephyr_library_sources(
            ${PLUGIN_ZVID_SOURCE_FILES}
        )
    endif()

    zephyr_linker_sources(DATA_SECTIONS ${MP_CORE_DIR}/mp_zephyr.ld)

    zephyr_iterable_section(NAME MpElementFactory GROUP DATA_REGION ${XIP_ALIGN_WITH_INPUT})

    zephyr_iterable_section(NAME MpPlugin GROUP DATA_REGION ${XIP_ALIGN_WITH_INPUT})

endif()
