# SPDX-License-Identifier: Apache-2.0
# Copyright 2026 NXP

add_library(dup_lib STATIC "")
target_sources(dup_lib PRIVATE
  reloc_dup.c
  ${CMAKE_CURRENT_SOURCE_DIR}/../src/multi/reloc_multi.c
)
target_compile_definitions(dup_lib PRIVATE RELOC_MULTI_IN_LIB)
target_link_libraries(dup_lib PUBLIC zephyr_interface)
add_dependencies(dup_lib zephyr_generated_headers)
