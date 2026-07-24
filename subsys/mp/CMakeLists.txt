#
# Copyright 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

cmake_minimum_required(VERSION 3.20.0)

if(CONFIG_MP)
    zephyr_library_named(mp)

    # Core framework sources
    zephyr_library_sources(mp_bin.c)
    zephyr_library_sources(mp_buffer.c)
    zephyr_library_sources(mp_bus.c)
    zephyr_library_sources(mp_caps.c)
    zephyr_library_sources(mp_dispatch.c)
    zephyr_library_sources(mp_element.c)
    zephyr_library_sources(mp_object.c)
    zephyr_library_sources(mp_pad.c)
    zephyr_library_sources(mp_parser.c)
    zephyr_library_sources(mp_pipeline.c)
    zephyr_library_sources(mp_sink.c)
    zephyr_library_sources(mp_src.c)
    zephyr_library_sources(mp_structure.c)
    zephyr_library_sources(mp_thread.c)
    zephyr_library_sources(mp_transform.c)
    zephyr_library_sources(mp_value.c)
    zephyr_library_sources_ifdef(CONFIG_MP_WORKQUEUE mp_workqueue.c)
    zephyr_library_sources_ifdef(CONFIG_MP_RPC       mp_transform_client.c)
    zephyr_library_sources_ifdef(CONFIG_MP_FAKE_SRC  mp_fake_src.c)

    # Auto-discover plugin and utility subdirectories. Each directory owns a
    # self-guarded CMakeLists.txt (it checks its own CONFIG_MP_* option), so
    # adding a new plugin needs no change to this file.
    file(GLOB MP_SUBDIRS RELATIVE ${CMAKE_CURRENT_LIST_DIR} ${CMAKE_CURRENT_LIST_DIR}/*)
    foreach(entry ${MP_SUBDIRS})
        if(IS_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/${entry}
           AND EXISTS ${CMAKE_CURRENT_LIST_DIR}/${entry}/CMakeLists.txt)
            add_subdirectory(${entry})
        endif()
    endforeach()
endif()
