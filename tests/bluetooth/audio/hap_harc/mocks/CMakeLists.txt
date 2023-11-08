#
# Copyright (c) 2023 Codecoup
#
# SPDX-License-Identifier: Apache-2.0
#
# CMakeLists.txt file for creating of mocks library.
#

add_library(local_mocks STATIC
  src/csip_set_coordinator.c
  src/has_client.c
)

target_include_directories(local_mocks PUBLIC
  include
)

add_subdirectory(${ZEPHYR_BASE}/tests/bluetooth/audio/mocks mocks)

target_link_libraries(local_mocks PUBLIC test_interface mocks)
