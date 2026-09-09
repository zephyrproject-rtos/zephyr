#
# Copyright 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

cmake_minimum_required(VERSION 3.20.0)

if(CONFIG_MPIPE)
  zephyr_library_named(mpipe)

  # Core framework sources
  zephyr_library_sources(mpipe_bin.c)
  zephyr_library_sources(mpipe_buffer.c)
  zephyr_library_sources(mpipe_element.c)
  zephyr_library_sources(mpipe_object.c)
  zephyr_library_sources(mpipe_pad.c)
  zephyr_library_sources(mpipe_parser.c)
  zephyr_library_sources(mpipe_pipeline.c)
  zephyr_library_sources(mpipe_sink.c)
  zephyr_library_sources(mpipe_src.c)
  zephyr_library_sources(mpipe_structure.c)
  zephyr_library_sources(mpipe_thread.c)
  zephyr_library_sources(mpipe_transform.c)
  zephyr_library_sources(mpipe_value.c)
  zephyr_library_sources_ifdef(CONFIG_MPIPE_WORKQUEUE mpipe_workqueue.c)
  zephyr_library_sources_ifdef(CONFIG_MPIPE_RPC  mpipe_transform_client.c)
  zephyr_library_sources_ifdef(CONFIG_MPIPE_FAKE_SRC  mpipe_fake_src.c)

  # Auto-discover plugin and utility subdirectories. Each directory owns a
  # self-guarded CMakeLists.txt (it checks its own CONFIG_MPIPE_* option), so
  # adding a new plugin needs no change to this file.
  file(GLOB MPIPE_SUBDIRS RELATIVE ${CMAKE_CURRENT_LIST_DIR} ${CMAKE_CURRENT_LIST_DIR}/*)
  foreach(entry ${MPIPE_SUBDIRS})
    if(IS_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/${entry}
      AND EXISTS ${CMAKE_CURRENT_LIST_DIR}/${entry}/CMakeLists.txt)
      add_subdirectory(${entry})
    endif()
  endforeach()
endif()
