# Copyright (c) 2025 Antmicro <www.antmicro.com>
# SPDX-License-Identifier: Apache-2.0

# This library provides a set of functions for creating FUSE structures

add_library(FUSE_CLIENT INTERFACE)
target_include_directories(FUSE_CLIENT INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

zephyr_library()
zephyr_library_sources(
  fuse_client.c
)

zephyr_library_link_libraries(FUSE_CLIENT)
