#/*
# * Copyright 2025 NXP
# *
# * SPDX-License-Identifier: Apache-2.0
# */

cmake_minimum_required(VERSION 3.20.0)

if(CONFIG_MP)
    set(MP_BASE_DIR ${CMAKE_CURRENT_LIST_DIR})
    set(MP_CORE_DIR ${MP_BASE_DIR}/src/core)

    message("Building MediaPipe library")

    find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

    zephyr_library_named(mp)

    zephyr_include_directories(
        ${MP_BASE_DIR}
    )

    file(GLOB MP_CORE_SOURCE_FILES
        ${MP_CORE_DIR}/*.c
    )

    # Built-in elements
    set(BUILTIN_ELEMENTS
        "mp_capsfilter.c:CONFIG_MP_CAPSFILTER"
    )

    foreach(ELEMENT_CONFIG ${BUILTIN_ELEMENTS})
        string(REPLACE ":" ";" ELEMENT_CONFIG_LIST ${ELEMENT_CONFIG})
        list(GET ELEMENT_CONFIG_LIST 0 ELEMENT_FILE)
        list(GET ELEMENT_CONFIG_LIST 1 ELEMENT_OPTION)

        if(NOT ${ELEMENT_OPTION})
            list(REMOVE_ITEM MP_CORE_SOURCE_FILES ${MP_CORE_DIR}/${ELEMENT_FILE})
        endif()
    endforeach()

    zephyr_library_sources(
        ${MP_CORE_SOURCE_FILES}
    )

endif()
