#
# Copyright 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

file(GLOB MP_CORE_SOURCE_FILES *.c)

set(BUILTIN_ELEMENTS
    "mp_capsfilter.c:CONFIG_MP_CAPSFILTER"
    "mp_transform_client.c:CONFIG_MP_RPC"
)

foreach(ELEMENT_CONFIG ${BUILTIN_ELEMENTS})
    string(REPLACE ":" ";" ELEMENT_CONFIG_LIST ${ELEMENT_CONFIG})
    list(GET ELEMENT_CONFIG_LIST 0 ELEMENT_FILE)
    list(GET ELEMENT_CONFIG_LIST 1 ELEMENT_OPTION)

    if(NOT ${ELEMENT_OPTION})
        list(REMOVE_ITEM MP_CORE_SOURCE_FILES ${CMAKE_CURRENT_LIST_DIR}/${ELEMENT_FILE})
    endif()
endforeach()

zephyr_library_sources(${MP_CORE_SOURCE_FILES})
