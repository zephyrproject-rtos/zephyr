# Copyright (c) 2025 Antmicro <www.antmicro.com>
# SPDX-License-Identifier: Apache-2.0

add_library(VIRTIOFS INTERFACE)
target_include_directories(VIRTIOFS INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

zephyr_library()
zephyr_library_sources(
  virtiofs.c
  virtiofs_zfs.c
)

zephyr_library_link_libraries(VIRTIOFS)
