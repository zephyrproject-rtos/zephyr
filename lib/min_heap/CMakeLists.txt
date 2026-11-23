# SPDX-License-Identifier: Apache-2.0

zephyr_interface_library_named(min_heap)

target_include_directories(min_heap INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

zephyr_library()

zephyr_library_sources(min_heap.c)

zephyr_library_link_libraries(min_heap)
