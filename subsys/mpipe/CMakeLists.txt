#
# Copyright 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_library_named(mpipe)

zephyr_library_sources(
  mpipe_bin.c
  mpipe_buffer.c
  mpipe_element.c
  mpipe_object.c
  mpipe_pad.c
  mpipe_parser.c
  mpipe_pipeline.c
  mpipe_sink.c
  mpipe_src.c
  mpipe_structure.c
  mpipe_thread.c
  mpipe_transform.c
  mpipe_value.c
)
zephyr_library_sources_ifdef(CONFIG_MPIPE_FAKE_SRC mpipe_fake_src.c)
zephyr_library_sources_ifdef(CONFIG_MPIPE_RPC mpipe_transform_client.c)
zephyr_library_sources_ifdef(CONFIG_MPIPE_WORKQUEUE mpipe_workqueue.c)

# Every plugin and utility directory ships its own CMakeLists.txt
file(GLOB mpipe_subdirs RELATIVE "${CMAKE_CURRENT_LIST_DIR}" "${CMAKE_CURRENT_LIST_DIR}/*")
foreach(entry ${mpipe_subdirs})
  if(IS_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/${entry}"
     AND EXISTS "${CMAKE_CURRENT_LIST_DIR}/${entry}/CMakeLists.txt")
    add_subdirectory("${entry}")
  endif()
endforeach()
