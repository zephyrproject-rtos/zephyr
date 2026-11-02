# Copyright (c) 2026 ITE Corporation.
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

zephyr_library_sources(
  it51xxx_crypto.c
)
