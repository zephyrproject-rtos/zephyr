# Copyright (c) 2025 Antmicro <www.antmicro.com>
# SPDX-License-Identifier: Apache-2.0

add_library(FUSE INTERFACE)
target_include_directories(FUSE INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

zephyr_library()
zephyr_library_sources(
  fuse.c
)

zephyr_library_link_libraries(FUSE)
