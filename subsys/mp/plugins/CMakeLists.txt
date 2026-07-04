#
# Copyright 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

file(GLOB PLUGIN_DIRS RELATIVE ${CMAKE_CURRENT_LIST_DIR} *)

foreach(plugin ${PLUGIN_DIRS})
    if(IS_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/${plugin})
        string(TOUPPER ${plugin} PLUGIN_UPPER)
        set(PLUGIN_CONFIG "CONFIG_MP_PLUGIN_${PLUGIN_UPPER}")
        add_subdirectory_ifdef(${PLUGIN_CONFIG} ${plugin})
    endif()
endforeach()
